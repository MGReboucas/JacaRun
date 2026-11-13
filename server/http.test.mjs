import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createHmac } from 'node:crypto';
import { Shop } from './shop.mjs';
import { createShopServer } from './http.mjs';

async function setup(t,enabled=true) {
  const provider={secret:'test-secret',collector:'123',live:false,checkout:async()=> 'https://sandbox.mercadopago.com.br/checkout/test',payment:async()=>provider.result};
  const shop=new Shop(':memory:',provider),token=shop.register().token;
  const server=createShopServer(shop,provider,{enabled});
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  t.after(async()=>{await new Promise(resolve=>{server.close(resolve);server.closeAllConnections();});shop.db.close();});
  return {shop,provider,base:`http://127.0.0.1:${server.address().port}`,headers:{'Content-Type':'application/json','Authorization':`Bearer ${token}`,'Idempotency-Key':'http-key-12345678'}};
}
test('HTTP purchase flow, validation, delivery and isolation',async t=>{
  const {base,provider,headers}=await setup(t);
  assert.equal((await fetch(base+'/v1/wallet')).status,401);
  assert.equal((await fetch(base+'/v1/orders',{method:'POST',headers,body:JSON.stringify({sku:'rays_2',cents:1})})).status,400);
  assert.equal((await fetch(base+'/v1/orders',{method:'POST',headers,body:'invalid-json'})).status,400);
  const response=await fetch(base+'/v1/orders',{method:'POST',headers,body:JSON.stringify({sku:'rays_2'})});
  assert.equal(response.status,200); const order=await response.json();
  assert.equal((await (await fetch(base+'/v1/wallet',{headers})).json()).rays,0);
  provider.result={id:42,external_reference:order.id,status:'approved',currency_id:'BRL',transaction_amount:2.99,collector_id:123,live_mode:false};
  const ts=String(Math.floor(Date.now()/1000)),request='http-123';
  const signature=createHmac('sha256',provider.secret).update(`id:42;request-id:${request};ts:${ts};`).digest('hex');
  const event={method:'POST',headers:{'Content-Type':'application/json','x-request-id':request,'x-signature':`ts=${ts},v1=${signature}`},body:JSON.stringify({type:'payment',data:{id:'42'}})};
  for(let i=0;i<2;i++) assert.equal((await fetch(base+'/webhooks/mercadopago?data.id=42',event)).status,200);
  assert.equal((await (await fetch(base+'/v1/wallet',{headers})).json()).rays,2);
  const other=await (await fetch(base+'/v1/sessions',{method:'POST',headers:{'Content-Type':'application/json'},body:'{}'})).json();
  assert.equal((await (await fetch(base+'/v1/wallet',{headers:{Authorization:`Bearer ${other.token}`}})).json()).rays,0);
  assert.equal((await fetch(base+'/webhooks/mercadopago?data.id=42',{...event,headers:{'Content-Type':'application/json'}})).status,401);
});
test('HTTP purchases disabled by default',async t=>{
  const {base,headers}=await setup(t,false);
  assert.equal((await fetch(base+'/v1/orders',{method:'POST',headers,body:'{}'})).status,503);
});
test('HTTP rejects huge bodies and throttles anonymous session creation',async t=>{
  const {base}=await setup(t);
  const headers={'Content-Type':'application/json'};
  assert.equal((await fetch(base+'/v1/sessions',{method:'POST',headers,body:JSON.stringify({data:'x'.repeat(9000)})})).status,413);
  let result;
  for(let i=0;i<5;i++) result=await fetch(base+'/v1/sessions',{method:'POST',headers,body:'{}'});
  assert.equal(result.status,429);
});
