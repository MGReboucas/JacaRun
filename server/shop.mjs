import { DatabaseSync } from 'node:sqlite';
import { randomBytes, randomUUID, createHash, createHmac, timingSafeEqual } from 'node:crypto';
import items from './catalog.json' with { type: 'json' };

export const catalog = Object.freeze(Object.fromEntries(items.map(item => [item.sku,Object.freeze(item)])));
export class Fault extends Error { constructor(status,message) { super(message); this.status=status; } }
const hash = token => createHash('sha256').update(token).digest('hex');
export function verifyWebhook(url, headers, secret, now=Date.now()) {
  const id=url.searchParams.get('data.id');
  const request=headers['x-request-id'];
  const signature=headers['x-signature'];
  if (!id || !/^\d{1,30}$/.test(id) || typeof request!=='string' || !/^[\w-]{1,128}$/.test(request) || typeof signature!=='string') throw new Fault(401,'Invalid signature');
  const parts=Object.fromEntries(signature.split(',').map(p=>p.trim().split('=')));
  if (!/^\d{10,13}$/.test(parts.ts ?? '') || !/^[a-f0-9]{64}$/i.test(parts.v1 ?? '')) throw new Fault(401,'Invalid signature');
  const timestamp=Number(parts.ts)*(parts.ts.length===10?1000:1);
  if (Math.abs(now-timestamp)>300000) throw new Fault(401,'Expired signature');
  const expected=createHmac('sha256',secret).update(`id:${id};request-id:${request};ts:${parts.ts};`).digest();
  if (!timingSafeEqual(expected,Buffer.from(parts.v1,'hex'))) throw new Fault(401,'Invalid signature');
  return id;
}
export class Shop {
  constructor(path, provider) {
    this.provider=provider;
    this.db=new DatabaseSync(path);
    this.db.exec(`PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON; PRAGMA busy_timeout=5000;
      CREATE TABLE IF NOT EXISTS players(id TEXT PRIMARY KEY, token TEXT UNIQUE NOT NULL, blocked INTEGER NOT NULL DEFAULT 0);
      CREATE TABLE IF NOT EXISTS orders(id TEXT PRIMARY KEY, player TEXT NOT NULL REFERENCES players(id), sku TEXT NOT NULL,
        request_key TEXT NOT NULL, cents INTEGER NOT NULL, rays INTEGER NOT NULL, coins INTEGER NOT NULL,
        checkout TEXT, state TEXT NOT NULL DEFAULT 'pending', created INTEGER NOT NULL, UNIQUE(player,request_key));
      CREATE TABLE IF NOT EXISTS grants(order_id TEXT PRIMARY KEY REFERENCES orders(id), payment TEXT UNIQUE NOT NULL,
        player TEXT NOT NULL REFERENCES players(id), rays INTEGER NOT NULL, coins INTEGER NOT NULL);
      CREATE INDEX IF NOT EXISTS grants_player ON grants(player);`);
  }
  register() {
    const token=randomBytes(32).toString('hex');
    this.db.prepare('INSERT INTO players(id,token) VALUES (?,?)').run(randomUUID(),hash(token));
    return {token};
  }
  authenticate(token) {
    if (typeof token!=='string' || !/^[a-f0-9]{64}$/.test(token)) throw new Fault(401,'Unauthorized');
    const player=this.db.prepare('SELECT * FROM players WHERE token=?').get(hash(token));
    if (!player) throw new Fault(401,'Unauthorized');
    if (player.blocked) throw new Fault(403,'Account requires support');
    return player.id;
  }
  wallet(player) {
    return this.db.prepare('SELECT COALESCE(SUM(rays),0) rays, COALESCE(SUM(coins),0) coins FROM grants WHERE player=?').get(player);
  }
  async order(player,sku,key) {
    if (!Object.hasOwn(catalog,sku) || typeof key!=='string' || !/^[a-zA-Z0-9_-]{16,80}$/.test(key)) throw new Fault(400,'Invalid product or key');
    let order=this.db.prepare('SELECT * FROM orders WHERE player=? AND request_key=?').get(player,key);
    if (order && order.sku!==sku) throw new Fault(409,'Key already used');
    if (order && order.state==='pending' && order.created+86400000<Date.now()) return {id:order.id,state:'expired',checkout:''};
    if (order && order.state==='pending' && order.created+86400000<Date.now()) return {id:order.id,state:'expired',checkout:''};
    if (!order) {
      const pending=this.db.prepare("SELECT COUNT(*) n FROM orders WHERE player=? AND state='pending' AND created>?").get(player,Date.now()-86400000);
      if (pending.n>=10) throw new Fault(429,'Too many pending orders');
      const item=catalog[sku], id=randomUUID();
      this.db.prepare('INSERT INTO orders(id,player,sku,request_key,cents,rays,coins,created) VALUES (?,?,?,?,?,?,?,?)').run(id,player,sku,key,item.cents,item.rays,item.coins,Date.now());
      order=this.db.prepare('SELECT * FROM orders WHERE id=?').get(id);
    }
    if (!order.checkout) {
      const checkout=await this.provider.checkout(order);
      const u=new URL(checkout);
      if (u.protocol!=='https:' || !['www.mercadopago.com.br','sandbox.mercadopago.com.br'].includes(u.hostname) || u.username || u.password || u.port) throw new Error('Invalid provider URL');
      this.db.prepare('UPDATE orders SET checkout=? WHERE id=?').run(checkout,order.id);
      order.checkout=checkout;
    }
    return {id:order.id,checkout:order.checkout,state:order.state};
  }
  async reconcile(paymentId) {
    const payment=await this.provider.payment(paymentId);
    if (String(payment.id)!==String(paymentId)) throw new Fault(400,'Payment ID mismatch');
    const order=this.db.prepare('SELECT * FROM orders WHERE id=?').get(payment.external_reference ?? '');
    if (!order) throw new Fault(400,'Unknown order');
    if (payment.currency_id!=='BRL' || !Number.isFinite(payment.transaction_amount) || Math.abs(payment.transaction_amount*100-order.cents)>0.001 ||
        String(payment.collector_id)!==String(this.provider.collector) || payment.live_mode!==this.provider.live) throw new Fault(400,'Payment mismatch');
    this.db.exec('BEGIN IMMEDIATE');
    try {
      if (['refunded','charged_back'].includes(payment.status) || Number(payment.transaction_amount_refunded)>0) {
        this.db.prepare('UPDATE players SET blocked=1 WHERE id=?').run(order.player);
        this.db.prepare("UPDATE orders SET state='review' WHERE id=?").run(order.id);
      } else if (payment.status==='approved') {
        const prior=this.db.prepare('SELECT * FROM grants WHERE payment=? OR order_id=?').get(String(payment.id),order.id);
        if (prior && (prior.order_id!==order.id || prior.payment!==String(payment.id))) throw new Fault(409,'Duplicate payment requires review');
        this.db.prepare('INSERT OR IGNORE INTO grants VALUES (?,?,?,?,?)').run(order.id,String(payment.id),order.player,order.rays,order.coins);
        this.db.prepare("UPDATE orders SET state='paid' WHERE id=? AND state!='review'").run(order.id);
      }
      this.db.exec('COMMIT');
    } catch(e) { this.db.exec('ROLLBACK'); throw e; }
  }
}

export class MercadoPago {
  constructor({token,secret,origin,collector,live}) { Object.assign(this,{token,secret,origin,collector,live}); }
  async api(path, options={}) {
    const response=await fetch('https://api.mercadopago.com'+path, {...options,redirect:'error',signal:AbortSignal.timeout(15000),
      headers:{Authorization:`Bearer ${this.token}`,'Content-Type':'application/json',...options.headers}});
    if (!response.ok) throw new Fault(502,'Payment provider unavailable');
    return response.json();
  }
  async checkout(order) {
    const p=await this.api('/checkout/preferences',{method:'POST',headers:{'X-Idempotency-Key':order.id},body:JSON.stringify({
      items:[{id:order.sku,title:`JacaRun - ${[order.rays && order.rays+' raios',order.coins && order.coins+' moedas'].filter(Boolean).join(' + ')}`,quantity:1,currency_id:'BRL',unit_price:order.cents/100}],
      external_reference:order.id,notification_url:this.origin+'/webhooks/mercadopago',
      back_urls:{success:this.origin+'/return',pending:this.origin+'/return',failure:this.origin+'/return'},
      expires:true,expiration_date_to:new Date(order.created+86400000).toISOString()
    })});
    return this.live?p.init_point:p.sandbox_init_point;
  }
  payment(id) { return this.api('/v1/payments/'+encodeURIComponent(id)); }
}
