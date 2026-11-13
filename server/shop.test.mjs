import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createHmac } from 'node:crypto';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { Shop, verifyWebhook, catalog } from './shop.mjs';

function fixture(t,path=':memory:') {
  const provider={collector:'123',live:false,checkout:async()=> 'https://sandbox.mercadopago.com.br/checkout/test',payment:async()=>provider.result};
  const shop=new Shop(path,provider); t.after(()=>shop.db.close());
  const {token}=shop.register(); const player=shop.authenticate(token);
  return {shop,provider,player,token};
}
async function paid(f,overrides={}) {
  const order=await f.shop.order(f.player,'coins_2000','test-key-12345678');
  f.provider.result={id:42,external_reference:order.id,status:'approved',currency_id:'BRL',transaction_amount:2.99,collector_id:123,live_mode:false,...overrides};
  return order;
}
test('authentication rejects missing, forged and blocked credentials',t=>{
  const f=fixture(t); for(const value of ['',undefined,'a'.repeat(64)]) assert.throws(()=>f.shop.authenticate(value));
  f.shop.db.prepare('UPDATE players SET blocked=1').run(); assert.throws(()=>f.shop.authenticate(f.token));
});
test('catalog and order idempotency belong to each player',async t=>{
  const f=fixture(t); const a=await paid(f); const b=await f.shop.order(f.player,'coins_2000','test-key-12345678'); assert.equal(a.id,b.id);
  await assert.rejects(f.shop.order(f.player,'rays_10','test-key-12345678'));
  await assert.rejects(f.shop.order(f.player,'__proto__','test-key-12345678'));
  await assert.rejects(f.shop.order(f.player,'coins_2000','x'));
  const other=f.shop.authenticate(f.shop.register().token);
  assert.notEqual((await f.shop.order(other,'coins_2000','test-key-12345678')).id,a.id);
  assert.equal(f.shop.wallet(other).coins,0);
});
test('repeated concurrent approval grants once',async t=>{
  const f=fixture(t); await paid(f); await Promise.all(Array.from({length:15},()=>f.shop.reconcile('42')));
  assert.equal(f.shop.wallet(f.player).coins,2000); assert.equal(f.shop.wallet(f.player).rays,0);
});
test('combos grant both currencies once at the advertised discounted prices',async t=>{
  const f=fixture(t);
  const combos=[['combo_small','rays_2','coins_2000',539],['combo_medium','rays_4','coins_4500',899],['combo_large','rays_10','coins_10000',1799]];
  let rays=0,coins=0,id=100;
  for(const [sku,raySku,coinSku,cents] of combos) {
    const item=catalog[sku], full=catalog[raySku].cents+catalog[coinSku].cents;
    assert.equal(item.cents,cents); assert.equal(item.rays,catalog[raySku].rays); assert.equal(item.coins,catalog[coinSku].coins);
    assert.ok(item.cents/full>=0.9 && item.cents/full<0.902);
    const order=await f.shop.order(f.player,sku,`combo-request-key-${id}`);
    f.provider.result={id,external_reference:order.id,status:'approved',currency_id:'BRL',transaction_amount:cents/100,collector_id:123,live_mode:false};
    await f.shop.reconcile(String(id)); await f.shop.reconcile(String(id));
    rays+=item.rays; coins+=item.coins;
    assert.equal(f.shop.wallet(f.player).rays,rays); assert.equal(f.shop.wallet(f.player).coins,coins);
    id++;
  }
});
test('pending/rejected/cancelled payment never grants',async t=>{
  const f=fixture(t); for(const status of ['pending','rejected','cancelled']) { await paid(f,{status}); await f.shop.reconcile('42'); }
  assert.equal(f.shop.wallet(f.player).coins,0);
});
test('all 27 packages have progressive unit prices and deliver the exact quantities once',async t=>{
  const f=fixture(t),items=Object.values(catalog);
  assert.equal(items.length,27);
  let expectedRays=0,expectedCoins=0,id=200;
  for(let i=0;i<9;i++) {
    const rays=items[i],coins=items[9+i],combo=items[18+i];
    assert.equal(combo.rays,rays.rays); assert.equal(combo.coins,coins.coins);
    const ratio=combo.cents/(rays.cents+coins.cents);
    assert.ok(ratio>=0.9 && ratio<0.902);
    if(i) {
      assert.ok(rays.rays>items[i-1].rays && rays.cents/rays.rays<items[i-1].cents/items[i-1].rays);
      assert.ok(coins.coins>items[8+i].coins && coins.cents/coins.coins<items[8+i].cents/items[8+i].coins);
    }
  }
  assert.equal(items[8].rays,100); assert.equal(items[17].coins,1000000);
  for(const item of items) {
    const order=await f.shop.order(f.player,item.sku,`expanded-request-${id}`);
    f.provider.result={id,external_reference:order.id,status:'approved',currency_id:'BRL',transaction_amount:item.cents/100,collector_id:123,live_mode:false};
    await f.shop.reconcile(String(id)); await f.shop.reconcile(String(id));
    expectedRays+=item.rays; expectedCoins+=item.coins;
    assert.equal(f.shop.wallet(f.player).rays,expectedRays);
    assert.equal(f.shop.wallet(f.player).coins,expectedCoins);
    ++id;
  }
});
test('amount, currency, seller, environment, reference and payment ID must match',async t=>{
  const f=fixture(t);
  for(const change of [{transaction_amount:0.01},{transaction_amount:2.991},{currency_id:'USD'},{collector_id:999},{live_mode:true},{external_reference:'foreign'},{id:99}]) {
    await paid(f,change); await assert.rejects(f.shop.reconcile('42')); assert.equal(f.shop.wallet(f.player).coins,0);
  }
});
test('refund blocks account and does not duplicate the grant',async t=>{
  const f=fixture(t); await paid(f); await f.shop.reconcile('42');
  f.provider.result.status='refunded'; await f.shop.reconcile('42'); assert.throws(()=>f.shop.authenticate(f.token));
  f.provider.result.status='approved'; await f.shop.reconcile('42'); assert.equal(f.shop.wallet(f.player).coins,2000);
});
test('a second payment for one order cannot grant again',async t=>{
  const f=fixture(t); await paid(f); await f.shop.reconcile('42'); f.provider.result.id=43;
  await assert.rejects(f.shop.reconcile('43')); assert.equal(f.shop.wallet(f.player).coins,2000);
});
test('failed checkout can be retried with same order',async t=>{
  const f=fixture(t); const original=f.provider.checkout; f.provider.checkout=async()=>{throw Error('timeout');};
  await assert.rejects(f.shop.order(f.player,'rays_2','test-key-12345678')); f.provider.checkout=original;
  await f.shop.order(f.player,'rays_2','test-key-12345678'); assert.equal(f.shop.db.prepare('SELECT COUNT(*) n FROM orders').get().n,1);
});
test('checkout rejects untrusted URLs',async t=>{
  const f=fixture(t); for(const url of ['http://www.mercadopago.com.br/x','https://evil.example/x','https://www.mercadopago.com.br@evil.example/x']) {
    f.provider.checkout=async()=>url; await assert.rejects(f.shop.order(f.player,'rays_2','test-key-12345678'));
  }
});
test('signed webhook rejects forged, stale or swapped IDs',()=>{
  const ts=String(Math.floor(Date.now()/1000)),secret='test-secret',request='request-123';
  const signature=createHmac('sha256',secret).update(`id:42;request-id:${request};ts:${ts};`).digest('hex');
  const headers={'x-request-id':request,'x-signature':`ts=${ts},v1=${signature}`};
  const url=new URL('https://shop.example/webhooks/mercadopago?data.id=42');
  assert.equal(verifyWebhook(url,headers,secret),'42');
  assert.throws(()=>verifyWebhook(url,headers,'wrong')); assert.throws(()=>verifyWebhook(url,headers,secret,Date.now()+600000));
  url.searchParams.set('data.id','43'); assert.throws(()=>verifyWebhook(url,headers,secret));
});
test('ledger survives reopening the database',async()=>{
  const dir=mkdtempSync(join(tmpdir(),'jacarun-shop-')); const path=join(dir,'test.sqlite');
  let shop;
  try {
    const provider={collector:'123',live:false,checkout:async()=> 'https://sandbox.mercadopago.com.br/checkout/test'};
    shop=new Shop(path,provider); const token=shop.register().token,player=shop.authenticate(token);
    const order=await shop.order(player,'rays_2','test-key-12345678');
    provider.payment=async()=>({id:42,external_reference:order.id,status:'approved',currency_id:'BRL',transaction_amount:2.99,collector_id:123,live_mode:false});
    await shop.reconcile('42'); shop.db.close(); shop=new Shop(path,provider); await shop.reconcile('42');
    assert.equal(shop.wallet(shop.authenticate(token)).rays,2);
  } finally { shop?.db.close(); rmSync(dir,{recursive:true}); }
});
