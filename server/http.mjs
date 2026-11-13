import http from 'node:http';
import { Fault, catalog, verifyWebhook } from './shop.mjs';

export function createShopServer(shop,provider,{enabled=false}={}) {
const limits=new Map();
const sweep=setInterval(()=>{const now=Date.now();for(const [key,value] of limits) if(value.until<now) limits.delete(key);},60000);
sweep.unref();
function limit(key,max) {
  const now=Date.now(); let value=limits.get(key);
  if (!value || value.until<now) { value={n:0,until:now+60000}; limits.set(key,value); }
  if (++value.n>max) throw new Fault(429,'Try again later');
}
async function body(req) {
  if (!(req.headers['content-type'] ?? '').startsWith('application/json')) throw new Fault(415,'JSON required');
  let value=''; for await (const chunk of req) { value+=chunk; if(Buffer.byteLength(value)>8192) throw new Fault(413,'Body too large'); }
  try { const result=JSON.parse(value); if (!result || Array.isArray(result) || typeof result!=='object') throw 0; return result; } catch { throw new Fault(400,'Invalid JSON'); }
}
const server=http.createServer(async(req,res)=>{
  res.setHeader('Cache-Control','no-store'); res.setHeader('X-Content-Type-Options','nosniff');
  res.setHeader('Content-Security-Policy',"default-src 'none'; frame-ancestors 'none'");
  const reply=(status,value)=>{res.writeHead(status,{'Content-Type':'application/json; charset=utf-8'});res.end(JSON.stringify(value));};
  try {
    limit(req.socket.remoteAddress,180);
    const url=new URL(req.url,'http://localhost');
    if(req.method==='GET' && url.pathname==='/return') return reply(200,{message:'Volte ao JacaRun e toque em VERIFICAR COMPRAS. O retorno do checkout nao confirma pagamento.'});
    if(req.method==='GET' && url.pathname==='/health') return reply(200,{ok:true});
    if(req.method==='POST' && url.pathname==='/webhooks/mercadopago') {
      const id=verifyWebhook(url,req.headers,provider.secret); const event=await body(req);
      if(event.type!=='payment' || String(event.data?.id)!==id) throw new Fault(400,'Invalid event');
      await shop.reconcile(id); return reply(200,{ok:true});
    }
    if(req.method==='POST' && url.pathname==='/v1/sessions') {
      limit('register:'+req.socket.remoteAddress,5); await body(req); return reply(201,shop.register());
    }
    const player=shop.authenticate((req.headers.authorization ?? '').replace(/^Bearer /,''));
    limit('player:'+player,60);
    if(req.method==='GET' && url.pathname==='/v1/catalog') return reply(200,{items:Object.values(catalog)});
    if(req.method==='GET' && url.pathname==='/v1/wallet') return reply(200,shop.wallet(player));
    if(req.method==='POST' && url.pathname==='/v1/orders') {
      if(!enabled) throw new Fault(503,'Purchases disabled');
      limit('orders:'+player,10); const data=await body(req);
      if(Object.keys(data).some(k=>k!=='sku')) throw new Fault(400,'Unexpected field');
      return reply(200,await shop.order(player,data.sku,req.headers['idempotency-key']));
    }
    throw new Fault(404,'Not found');
  } catch(e) { if(!e.status) console.error('Shop internal error:',e.name); reply(e.status ?? 500,{error:e.status?e.message:'Internal error'}); }
});
server.requestTimeout=20000; server.headersTimeout=10000;
server.on('close',()=>clearInterval(sweep));
return server;
}
