// Cyclone Target Lock - web control panel (single page, served from flash)
#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cyclone Target Lock</title>
<style>
:root{--bg:#0a0d1f;--card:#131836;--card2:#1b2148;--line:#2a3266;--txt:#e8ecff;--dim:#8d97c8;--cy:#00e5ff;--pk:#ff3d9a;--gr:#20e070;--rd:#ff4560;--yl:#ffc93c}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,Segoe UI,Arial,sans-serif;background:radial-gradient(1200px 600px at 20% -10%,#1d1b4d 0,var(--bg) 60%) fixed;color:var(--txt);padding:14px 14px 90px}
.wrap{max-width:720px;margin:auto}
h1{margin:4px 0 2px;font-size:22px;background:linear-gradient(90deg,var(--cy),#b56cff);-webkit-background-clip:text;background-clip:text;color:transparent}
.sub{color:var(--dim);font-size:13px;margin-bottom:12px;display:flex;gap:8px;align-items:center}
.dot{width:9px;height:9px;border-radius:50%;background:var(--rd);display:inline-block}
.dot.ok{background:var(--gr);box-shadow:0 0 8px var(--gr)}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:14px;margin:12px 0}
.hero{display:grid;grid-template-columns:190px 1fr;gap:12px;align-items:center}
@media(max-width:480px){.hero{grid-template-columns:1fr;justify-items:center}}
canvas{width:190px;height:190px}
.stats{width:100%}
.big{font-size:54px;font-weight:800;line-height:1;font-variant-numeric:tabular-nums}
.chip{display:inline-block;font-size:11px;letter-spacing:1px;text-transform:uppercase;padding:3px 9px;border-radius:99px;background:var(--card2);color:var(--cy);border:1px solid var(--line)}
.chip.playing{color:#04140a;background:var(--gr);border-color:var(--gr)}
.chip.ending,.chip.fx{color:#1a1200;background:var(--yl);border-color:var(--yl)}
.grid3{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:10px 0}
.kv{background:var(--card2);border-radius:12px;padding:8px;text-align:center}
.kv b{display:block;font-size:22px}
.kv span{font-size:11px;color:var(--dim);text-transform:uppercase;letter-spacing:1px}
.bar{height:8px;background:var(--card2);border-radius:9px;overflow:hidden;margin:8px 0}
.bar i{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--cy),var(--pk));transition:width .4s linear}
button{font:inherit;border:0;border-radius:11px;padding:11px 16px;color:#fff;background:#2d3775;cursor:pointer;font-weight:600}
button:active{transform:scale(.97)}
button.go{background:#12a150}button.no{background:#c62839}button.ghost{background:transparent;border:1px solid var(--line);color:var(--dim)}
button.sm{padding:6px 11px;font-size:13px}
.btns{display:flex;gap:8px;flex-wrap:wrap;margin-top:8px}.btns button{flex:1}
nav{display:flex;gap:6px;overflow-x:auto;padding:2px 0 6px;position:sticky;top:0;background:linear-gradient(var(--bg) 70%,transparent);z-index:5}
nav button{flex:none;background:var(--card);border:1px solid var(--line);color:var(--dim);padding:9px 14px}
nav button.on{background:linear-gradient(90deg,#1c6cff,#8a3dff);color:#fff;border-color:transparent}
.tab{display:none}.tab.on{display:block}
.h{margin:16px 0 4px;font-size:12px;letter-spacing:1.5px;text-transform:uppercase;color:var(--cy);display:flex;justify-content:space-between;align-items:center}
.row{display:grid;grid-template-columns:1fr auto;gap:2px 10px;align-items:center;padding:7px 0;border-bottom:1px solid #1c2350}
.row label{font-size:14px}.row output{font-weight:700;color:var(--cy);font-variant-numeric:tabular-nums;min-width:56px;text-align:right}
.row input[type=range]{grid-column:1/-1;width:100%;accent-color:var(--pk)}
.row select{background:var(--card2);color:var(--txt);border:1px solid var(--line);border-radius:9px;padding:8px}
.row input[type=color]{width:56px;height:34px;border:0;background:none;padding:0}
.sw{position:relative;width:46px;height:26px}.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;inset:0;background:var(--card2);border:1px solid var(--line);border-radius:99px;transition:.2s}
.sw span:before{content:"";position:absolute;width:18px;height:18px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.2s}
.sw input:checked+span{background:var(--gr)}.sw input:checked+span:before{transform:translateX(20px)}
.note{color:var(--dim);font-size:12px;padding:6px 0}
.pgrid{display:grid;gap:8px}
.pc{background:var(--card2);border:1px solid var(--line);border-radius:12px;padding:10px;display:flex;gap:10px;align-items:center;justify-content:space-between}
.pc small{display:block;color:var(--dim);margin-top:2px}
#toast{position:fixed;left:50%;bottom:22px;transform:translateX(-50%) translateY(80px);background:#0e1330;border:1px solid var(--cy);color:var(--txt);padding:10px 18px;border-radius:99px;transition:.3s;z-index:20;font-size:14px}
#toast.show{transform:translateX(-50%) translateY(0)}
.chip.combo{color:#1a1200;background:var(--yl);border-color:var(--yl);margin-left:6px;display:none}
.lb{width:100%;border-collapse:collapse;font-variant-numeric:tabular-nums}
.lb td,.lb th{padding:8px 6px;border-bottom:1px solid #1c2350;text-align:left;font-size:14px}
.lb th{font-size:11px;color:var(--dim);text-transform:uppercase;letter-spacing:1px;font-weight:600}
.lb td:first-child{color:var(--cy);font-weight:800;width:34px}
.lb tr.me td{background:#1c6cff22}
.fld{display:grid;gap:4px;padding:7px 0}
.fld label{font-size:13px;color:var(--dim)}
.fld input{font:inherit;background:var(--card2);color:var(--txt);border:1px solid var(--line);border-radius:9px;padding:9px}
.prog{height:8px;background:var(--card2);border-radius:9px;overflow:hidden;margin:8px 0;display:none}
.prog i{display:block;height:100%;width:0;background:var(--gr)}
code{font-size:12px;color:var(--cy)}
#modal{position:fixed;inset:0;background:#000a;display:none;align-items:center;justify-content:center;z-index:30;padding:16px}
#modal.show{display:flex}
#modal .card{max-width:340px;width:100%;text-align:center}
#modal input{font:800 34px/1 system-ui;width:150px;text-align:center;letter-spacing:10px;text-transform:uppercase;background:var(--card2);color:var(--txt);border:1px solid var(--line);border-radius:12px;padding:10px;margin:10px 0}
footer{text-align:center;color:var(--dim);font-size:12px;margin-top:22px}
footer a{color:var(--cy);text-decoration:none}
</style>
</head>
<body>
<div class="wrap">

<h1>&#127744; Cyclone Target Lock</h1>
<div class="sub"><span class="dot" id="conn"></span><span id="connTxt">connecting&hellip;</span><span>&middot;</span><span id="saved">&nbsp;</span></div>

<div class="card hero">
  <canvas id="ring" width="380" height="380"></canvas>
  <div class="stats">
    <div><span class="chip" id="mode">idle</span><span class="chip combo" id="combo"></span></div>
    <div class="big" id="time">--</div>
    <div class="bar"><i id="tbar"></i></div>
    <div class="grid3">
      <div class="kv"><b id="score">0</b><span id="l1">Score</span></div>
      <div class="kv"><b id="best">0</b><span id="l2">Best</span></div>
      <div class="kv"><b id="level">1</b><span>Level</span></div>
    </div>
    <div class="btns">
      <button class="go" onclick="act('start')">&#9654; START</button>
      <button class="no" onclick="act('stop')">&#9632; STOP</button>
      <button class="ghost" onclick="act('reset')">&#8635; RESET</button>
    </div>
  </div>
</div>

<nav id="nav"></nav>
<div id="tabs"></div>

<footer>Made by <a href="https://github.com/Am4l-babu" target="_blank">Am4l-babu</a> &middot; Cyclone Target Lock v2</footer>
</div>
<div id="toast"></div>
<div id="modal"><div class="card">
  <div class="h" style="justify-content:center">New high score!</div>
  <div id="mTxt" class="note"></div>
  <input id="mName" maxlength="3" autocomplete="off" placeholder="AAA">
  <div class="btns"><button class="go" id="mSave">Save</button><button class="ghost" id="mSkip">Skip</button></div>
</div></div>

<script>
const $=s=>document.querySelector(s);
let S={},L={},M={},N={},slots=[],dirty={},timer,ip='',sv=-1,lbv=-1,SC=null,skipPending=-1,ws=null,pollT=null;

// ---------- UI schema: [type,key,label,unit,extra] ----------
const TABS={
Game:[
['h','Round'],
['range','roundSeconds','Round time','s'],
['range','winScore','Win when score reaches (0 = off)',''],
['range','timeBonus','Time bonus per hit','s'],
['range','timePenalty','Time penalty per miss','s'],
['h','Speed'],
['range','speedDelay','Start speed (ms per step, lower = faster)','ms'],
['range','minDelay','Fastest speed','ms'],
['range','speedStep','Speed-up per point','ms'],
['range','pointsPerLevel','Points per level',''],
['h','Scoring'],
['range','hitPoints','Points per hit',''],
['range','missPenalty','Penalty per miss',''],
['range','hitWindow','Hit window (+/- LEDs)',''],
['h','Ring'],
['range','ledCount','Number of LEDs',''],
['range','brightness','Brightness',''],
['select','dirMode','Direction','','dir'],
['h','Players'],
['select','playerMode','Mode','','players'],
['note','Duel: a second button on D3 plays against the first on the same cursor. Turns: two players share one button, one round each.'],
['range','comboEvery','Combo: multiplier +1 every N hits in a row (0 = off, max x4)','']],
Look:[
['h','Running cursor'],
['color','cCursor','Cursor colour'],
['color','cTarget','Target colour'],
['color','cBg','Background colour'],
['range','tailLen','Comet tail length','LEDs'],
['toggle','targetPulse','Pulsing target'],
['color','cP2','Player 2 hit colour (duel)'],
['h','Start sweep'],
['color','cStart','Sweep colour'],
['range','startSweep','Sweep speed (0 = none)','ms/LED'],
['test','start','&#9654; Test start sweep'],
['h','Idle / attract mode'],
['select','idleMode','Animation','','idle'],
['color','cIdle','Idle colour'],
['range','idleSpeed','Chase step','ms']],
Effects:[
['h','On every HIT','hit'],
['select','hitStyle','Style','','styles'],
['range','hitBlinks','Blink count',''],
['range','hitMs','Blink time','ms'],
['color','cHit','Colour'],
['h','On every MISS','miss'],
['select','missStyle','Style','','styles'],
['range','missBlinks','Blink count',''],
['range','missMs','Blink time','ms'],
['color','cMiss','Colour'],
['h','GAME OVER (time up)','over'],
['select','overStyle','Style','','styles'],
['range','overBlinks','Blink count',''],
['range','overMs','Blink time','ms'],
['color','cOver','Colour'],
['h','WIN / NEW BEST','win'],
['select','winStyle','Style','','styles'],
['range','winBlinks','Blink count',''],
['range','winMs','Blink time','ms'],
['color','cWin','Colour'],
['note','Effect length = blinks x 2 x blink time (hit/miss are capped at 2 s, game over / win at 15 s). Set blinks to 0 to switch an effect off.']],
Sound:[
['toggle','buzzer','Buzzer on'],
['h','Sound effects'],
['snd','sndStart','Round start'],
['snd','sndHit','Hit'],
['snd','sndLevel','Level up'],
['snd','sndMiss','Miss'],
['snd','sndOver','Game over'],
['snd','sndWin','Win / new best'],
['h','Countdown'],
['snd','sndTick','Tick sound'],
['range','tickSec','Tick during last N seconds (0 = off)','s'],
['toggle','stepClick','Click on every cursor step']],
Scores:[['scores']],
Presets:[['presets']],
System:[['system']]
};

// ---------- helpers ----------
function toast(t){const e=$('#toast');e.textContent=t;e.classList.add('show');clearTimeout(toast.t);toast.t=setTimeout(()=>e.classList.remove('show'),1800)}
function conn(ok){$('#conn').className='dot'+(ok?' ok':'');$('#connTxt').textContent=ok?'connected':'offline'}
async function api(p,o){try{const r=await fetch(p,o);const j=await r.json();conn(true);if(!r.ok)throw j;return j}catch(e){if(e&&e.error){if(e.error!=='pin')toast(e.error)}else conn(false);throw e}}
// admin calls: add the PIN, ask for it if the device wants one
function getPin(){try{return sessionStorage.getItem('pin')||''}catch(e){return ''}}
function putPin(v){try{v?sessionStorage.setItem('pin',v):sessionStorage.removeItem('pin')}catch(e){}}
async function admin(path,body){
 for(let tries=0;tries<3;tries++){
  const b=new URLSearchParams(body||{});b.set('pin',getPin());
  try{return await api(path,{method:'POST',body:b})}
  catch(e){if(!e||e.error!=='pin')throw e;const v=prompt(tries?'Wrong PIN. Admin PIN:':'Admin PIN:');if(v===null)throw e;putPin(v.trim())}}
 toast('Wrong PIN');throw {error:'pin'}}
function set(k,v){S[k]=v;dirty[k]=v;$('#saved').textContent='saving…';clearTimeout(timer);timer=setTimeout(flush,250)}
async function flush(){if(!Object.keys(dirty).length)return;const b=new URLSearchParams(dirty);dirty={};try{const j=await api('/api/set',{method:'POST',body:b});if(j.sv!==undefined)sv=j.sv}catch(e){}}
async function act(a){try{const j=await api('/api/'+a);draw(j)}catch(e){}}
async function test(kind){await flush();try{await api('/api/test?fx='+kind)}catch(e){}}
async function snd(id){try{await api('/api/test?sound='+id)}catch(e){}}

// ---------- build controls ----------
function mk(t,c,h){const e=document.createElement(t);if(c)e.className=c;if(h!==undefined)e.innerHTML=h;return e}
function control(sp){
 const [t,k,l,u,x]=sp;
 if(t==='h'){const d=mk('div','h',k);if(l){const b=mk('button','sm ghost','&#9654; Test');b.onclick=()=>test(l);d.appendChild(b)}return d}
 if(t==='note')return mk('div','note',k);
 if(t==='test'){const b=mk('button','',l);b.style.marginTop='8px';b.onclick=()=>test(k);return b}
 const r=mk('div','row');r.appendChild(mk('label','',l));
 if(t==='range'){const o=mk('output');o.dataset.o=k;r.appendChild(o);const i=mk('input');i.type='range';i.dataset.k=k;const lm=L[k]||[0,255];i.min=lm[0];i.max=lm[1];i.oninput=()=>{o.textContent=i.value+(u?' '+u:'');set(k,+i.value);pv()};r.appendChild(i)}
 else if(t==='color'){const i=mk('input');i.type='color';i.dataset.k=k;i.oninput=()=>{set(k,i.value);pv()};r.appendChild(i)}
 else if(t==='toggle'){const w=mk('label','sw');const i=mk('input');i.type='checkbox';i.dataset.k=k;i.onchange=()=>{set(k,i.checked?1:0);pv()};w.appendChild(i);w.appendChild(mk('span'));r.appendChild(w)}
 else if(t==='select'||t==='snd'){const s=mk('select');s.dataset.k=k;(M[x||'sounds']||[]).forEach((n,i)=>{const o=mk('option','',n);o.value=i;s.appendChild(o)});s.onchange=()=>{set(k,+s.value);pv()};
  if(t==='snd'){const w=mk('div');w.style.display='flex';w.style.gap='6px';const b=mk('button','sm','&#9654;');b.onclick=()=>snd(s.value);w.appendChild(s);w.appendChild(b);r.appendChild(w)}else r.appendChild(s)}
 return r}
function presets(box){
 box.appendChild(mk('div','h','Built-in presets'));
 const g=mk('div','pgrid');
 (M.presets||[]).forEach((p,i)=>{const c=mk('div','pc','<div><b>'+p[0]+'</b><small>'+p[1]+'</small></div>');const b=mk('button','sm','Apply');b.onclick=async()=>{await api('/api/preset?id='+i);await load();toast(p[0]+' applied')};c.appendChild(b);g.appendChild(c)});
 box.appendChild(g);
 box.appendChild(mk('div','h','My presets (saved on the device)'));
 const g2=mk('div','pgrid');
 for(let i=0;i<3;i++){const c=mk('div','pc','<div><b>Slot '+(i+1)+'</b><small>'+(slots[i]?'saved':'empty')+'</small></div>');const w=mk('div');w.style.display='flex';w.style.gap='6px';
  const sv=mk('button','sm','Save');sv.onclick=async()=>{await flush();await api('/api/slot?op=save&i='+i);slots[i]=true;build();toast('Saved to slot '+(i+1))};
  const ld=mk('button','sm ghost','Load');ld.onclick=async()=>{try{await api('/api/slot?op=load&i='+i);await load();toast('Slot '+(i+1)+' loaded')}catch(e){toast('Slot is empty')}};
  w.appendChild(sv);w.appendChild(ld);c.appendChild(w);g2.appendChild(c)}
 box.appendChild(g2)}
function system(box){
 box.appendChild(mk('div','h','Device'));
 box.appendChild(mk('div','note','<span id="info"></span>'));
 box.appendChild(mk('div','h','Backup'));
 const b1=mk('button','sm','Export settings');b1.onclick=()=>{const a=document.createElement('a');a.href='data:application/json,'+encodeURIComponent(JSON.stringify(S,null,1));a.download='cyclone-settings.json';a.click()};
 const f=mk('input');f.type='file';f.accept='.json';f.style.display='none';f.onchange=async()=>{try{const j=JSON.parse(await f.files[0].text());await api('/api/set',{method:'POST',body:new URLSearchParams(j)});await load();toast('Settings imported')}catch(e){toast('Bad file')}};
 const b2=mk('button','sm ghost','Import settings');b2.onclick=()=>f.click();
 const w=mk('div','btns');w.appendChild(b1);w.appendChild(b2);w.appendChild(f);box.appendChild(w);
 box.appendChild(mk('div','h','Danger zone'));
 const d=mk('div','btns');
 const rh=mk('button','no sm','Reset scores');rh.onclick=async()=>{if(confirm('Reset the high score and the leaderboard?')){try{await admin('/api/resethigh');toast('Scores reset')}catch(e){}}};
 const fr=mk('button','no sm','Factory reset');fr.onclick=async()=>{if(confirm('Restore ALL game settings to defaults? (WiFi and PIN are kept)')){try{await admin('/api/factory');await load();toast('Defaults restored')}catch(e){}}};
 const rb=mk('button','ghost sm','Reboot');rb.onclick=async()=>{if(confirm('Reboot the device?')){try{await admin('/api/reboot');toast('Rebooting…')}catch(e){}}};
 d.appendChild(rh);d.appendChild(fr);d.appendChild(rb);box.appendChild(d);
 netBox(box)}

function scores(box){
 box.appendChild(mk('div','h','Leaderboard (solo + turns)'));
 const t=mk('table','lb');t.id='lbT';box.appendChild(t);
 box.appendChild(mk('div','h','Last round'));
 box.appendChild(mk('div','note','<span id="lastR">No round played yet.</span>'))}
function esc(s){return String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function renderScores(){
 const t=$('#lbT');if(!t||!SC)return;
 let h='<tr><th>#</th><th>Name</th><th>Score</th><th>Hit %</th><th>Streak</th></tr>';
 if(!SC.lb.length)h+='<tr><td></td><td colspan="4" style="color:var(--dim)">No scores yet. Play a round!</td></tr>';
 SC.lb.forEach((e,i)=>{h+='<tr'+(i===SC.pending?' class="me"':'')+'><td>'+(i+1)+'</td><td>'+esc(e.n)+'</td><td><b>'+e.s+'</b></td><td>'+e.a+'%</td><td>'+e.k+'</td></tr>'});
 t.innerHTML=h;
 const R=SC.last,el=$('#lastR');if(!R||!el)return;
 const pl=p=>'score <b>'+p.score+'</b> &middot; '+p.hits+' hits / '+p.misses+' misses ('+p.acc+'%) &middot; best streak '+p.streak+
  (p.misses?' &middot; misses: '+p.early+' early, '+p.late+' late'+(p.early>p.late*2?' <i>(wait a touch longer)</i>':p.late>p.early*2?' <i>(press a touch sooner)</i>':''):'');
 if(R.players===0)el.innerHTML=pl(R.p[0]);
 else if(R.players===2&&R.winner<0&&!R.p[1].hits&&!R.p[1].misses)el.innerHTML='Player 1: '+pl(R.p[0])+'<br>Player 2 is up next.';
 else el.innerHTML=(R.winner<0?'<b>Draw</b>':'<b>Player '+(R.winner+1)+' wins</b>')+'<br>P1: '+pl(R.p[0])+'<br>P2: '+pl(R.p[1])}
async function loadScores(){try{SC=await api('/api/scores');renderScores();askName()}catch(e){}}
function askName(){
 if(!SC||SC.pending<0||SC.pending===skipPending||$('#modal').classList.contains('show'))return;
 $('#mTxt').textContent='Rank #'+(SC.pending+1)+' with '+SC.lb[SC.pending].s+' points. Enter your initials:';
 const i=$('#mName');i.value='';$('#modal').classList.add('show');setTimeout(()=>i.focus(),50)}
$('#mSave').onclick=async()=>{const v=$('#mName').value.trim();if(!v){toast('Type 1 to 3 letters');return}
 try{SC=await api('/api/name?n='+encodeURIComponent(v));closeName();toast('Saved!')}catch(e){closeName()}};
function closeName(){$('#modal').classList.remove('show');$('#mName').blur();renderScores()}
$('#mSkip').onclick=()=>{skipPending=SC?SC.pending:-1;closeName()};
$('#mName').addEventListener('keydown',e=>{if(e.key==='Enter')$('#mSave').click()});
function netBox(box){
 box.appendChild(mk('div','h','WiFi &amp; security'));
 const st=N.staOk?'connected, IP <b>'+N.staIp+'</b>':N.staGaveUp?'not found, check the name and password (reboot to retry)':(N.sta?'connecting…':'off');
 box.appendChild(mk('div','note','Hotspot <b>'+esc(N.ap||'')+'</b> ('+(N.apIp||'')+')<br>Home WiFi: '+(N.sta?esc(N.sta)+', ':'')+st+'<br>Address: <b>http://'+esc(N.host||'')+'.local</b><br>Admin PIN: '+(N.pin?'<b>on</b>':'off (anyone on the WiFi can reset or reboot)')+'<br>Firmware built: '+esc(N.fw||'')));
 const f=(id,l,t,ph,v)=>{const d=mk('div','fld');d.appendChild(mk('label','',l));const i=mk('input');i.id=id;i.type=t;i.placeholder=ph||'';if(v!==undefined)i.value=v;i.autocomplete='off';d.appendChild(i);box.appendChild(d)};
 f('nAp','Hotspot password (8 to 32 characters)','password','leave empty to keep');
 f('nSsid','Home WiFi name (empty = hotspot only)','text','',N.sta||'');
 f('nPass','Home WiFi password','password','leave empty to keep');
 f('nHost','Device name (http://name.local)','text','',N.host||'');
 f('nPin','New admin PIN (4 to 8 digits)','password','leave empty to keep');
 const w=mk('div','btns');
 const sv2=mk('button','go sm','Save and reboot');sv2.onclick=async()=>{
  const b={staSsid:$('#nSsid').value.trim(),host:$('#nHost').value.trim().toLowerCase()};
  if($('#nAp').value)b.apPass=$('#nAp').value;if($('#nPass').value)b.staPass=$('#nPass').value;if($('#nPin').value)b.newPin=$('#nPin').value.trim();
  if(!confirm('Save and reboot the device? If you changed the hotspot password, rejoin with the new one.'))return;
  try{await admin('/api/net',b);if(b.newPin)putPin(b.newPin);toast('Saved. Rebooting…');setTimeout(()=>location.reload(),9000)}catch(e){}};
 const rp=mk('button','ghost sm','Remove PIN');rp.onclick=async()=>{if(!N.pin){toast('No PIN is set');return}if(!confirm('Remove the admin PIN and reboot?'))return;try{await admin('/api/net',{newPin:''});putPin('');toast('PIN removed. Rebooting…');setTimeout(()=>location.reload(),9000)}catch(e){}};
 w.appendChild(sv2);w.appendChild(rp);box.appendChild(w);
 box.appendChild(mk('div','note','Forgot the PIN? Hold the game button while powering on for 3 s: the PIN, hotspot password and home WiFi are cleared.'));
 box.appendChild(mk('div','h','Firmware update'));
 box.appendChild(mk('div','note','Build with PlatformIO (<code>.pio/build/game/firmware.bin</code>) or the Arduino IDE (<i>Sketch &rarr; Export Compiled Binary</i>) and upload the .bin here. Settings and scores are kept.'));
 const fi=mk('input');fi.type='file';fi.accept='.bin';fi.style.display='none';
 const pg=mk('div','prog');pg.appendChild(mk('i'));
 const ub=mk('button','sm','Choose firmware .bin');ub.onclick=()=>fi.click();
 fi.onchange=()=>{const file=fi.files[0];fi.value='';if(!file)return;if(!confirm('Upload '+file.name+' ('+Math.round(file.size/1024)+' KB)?'))return;
  let pin='';if(N.pin){pin=getPin()||prompt('Admin PIN:')||'';putPin(pin)}
  const x=new XMLHttpRequest();x.open('POST','/update');if(N.pin)x.setRequestHeader('Authorization','Basic '+btoa('admin:'+pin));
  pg.style.display='block';pg.firstChild.style.width='0';x.upload.onprogress=e=>{if(e.lengthComputable)pg.firstChild.style.width=(100*e.loaded/e.total)+'%'};
  x.onload=()=>{pg.style.display='none';if(x.status===200&&/success/i.test(x.responseText)){toast('Updated! Rebooting…');setTimeout(()=>location.reload(),12000)}else{toast(x.status===401?'Wrong PIN':'Update failed');if(x.status===401)putPin('')}};
  x.onerror=()=>{toast('Upload failed');pg.style.display='none'};
  const fd=new FormData();fd.append('firmware',file,file.name);x.send(fd)};
 const w2=mk('div','btns');w2.appendChild(ub);w2.appendChild(fi);box.appendChild(w2);box.appendChild(pg);
 box.appendChild(mk('div','note','Or from PlatformIO over WiFi: <code>pio run -e game -t upload --upload-port '+esc(N.host||'cyclone')+'.local</code>'+(N.pin?' plus <code>--upload-flags=--auth=YOURPIN</code>':'')))}

let cur='Game';
function build(){
 const nav=$('#nav'),tabs=$('#tabs');nav.innerHTML='';tabs.innerHTML='';
 Object.keys(TABS).forEach(n=>{
  const b=mk('button',n===cur?'on':'',n);b.onclick=()=>{cur=n;build()};nav.appendChild(b);
  const c=mk('div','card tab'+(n===cur?' on':''));
  TABS[n].forEach(sp=>{if(sp[0]==='presets')presets(c);else if(sp[0]==='system')system(c);else if(sp[0]==='scores')scores(c);else c.appendChild(control(sp))});
  tabs.appendChild(c)});
 renderScores();refresh()}
function refresh(){
 document.querySelectorAll('[data-k]').forEach(e=>{const v=S[e.dataset.k];if(v===undefined)return;if(e.type==='checkbox')e.checked=!!v;else e.value=v});
 document.querySelectorAll('[data-o]').forEach(o=>{const i=document.querySelector('input[data-k="'+o.dataset.o+'"]');if(!i)return;const u=(TABS.Game.concat(TABS.Look,TABS.Effects,TABS.Sound).find(s=>s[1]===o.dataset.o)||[])[3];o.textContent=i.value+(u?' '+u:'')});
 pv()}
async function load(){const d=await api('/api/settings');S=d.v;L=d.limits;M=d.meta;slots=d.slots;N=d.net||{};ip=N.apIp||'';sv=d.sv;build()}
// another phone changed something: pull the new values without rebuilding the page
async function resync(){if(Object.keys(dirty).length)return;try{const d=await api('/api/settings');S=d.v;N=d.net||{};sv=d.sv;
 const sl=JSON.stringify(slots)!==JSON.stringify(d.slots);slots=d.slots;if(sl&&cur==='Presets')build();else refresh()}catch(e){}}

// ---------- live status ----------
function fmt(s){return s>=100?Math.floor(s/60)+':'+String(s%60).padStart(2,'0'):s+'s'}
function draw(j){
 const m=$('#mode');m.textContent=j.mode;m.className='chip '+j.mode;
 $('#time').textContent=fmt(j.timeLeft);$('#level').textContent=j.level;
 const duel=j.players===1,turns=j.players===2;
 if(duel){$('#l1').textContent='Player 1';$('#score').textContent=j.p[0];$('#l2').textContent='Player 2';$('#best').textContent=j.p[1]}
 else{$('#l1').textContent=turns?'P'+(j.turn+1)+' score':'Score';$('#score').textContent=j.score;$('#l2').textContent='Best';$('#best').textContent=j.best}
 const mu=duel?Math.max(j.mult[0],j.mult[1]):j.mult[turns?j.turn:0],c=$('#combo');c.style.display=mu>1&&(j.mode==='playing'||j.mode==='fx')?'inline-block':'none';c.textContent='x'+mu+' combo';
 if(sv>=0&&j.sv!==sv){sv=j.sv;resync()}
 if(j.lbv!==lbv){lbv=j.lbv;loadScores()}
 $('#tbar').style.width=Math.min(100,100*j.timeLeft/Math.max(1,j.round))+'%';
 $('#saved').textContent=j.saved?'settings saved ✓':'saving…';
 const i=$('#info');if(i)i.innerHTML='IP: '+ip+'<br>Clients: '+j.clients+'<br>Free heap: '+j.heap+' B<br>Uptime: '+fmt(j.up)}
async function poll(){try{draw(await api('/api/state'))}catch(e){}}
// live updates: WebSocket on port 81, polling while it is down
function live(){
 if(!pollT)pollT=setInterval(poll,700);
 try{ws=new WebSocket('ws://'+location.hostname+':81/')}catch(e){setTimeout(live,3000);return}
 ws.onopen=()=>{clearInterval(pollT);pollT=null;conn(true)};
 ws.onmessage=e=>{try{draw(JSON.parse(e.data))}catch(x){}};
 ws.onclose=()=>{ws=null;conn(false);if(!pollT)pollT=setInterval(poll,700);setTimeout(live,3000)}}

// ---------- ring preview (simulated with your current settings) ----------
const cv=$('#ring'),g=cv.getContext('2d');let pos=0,tgt=7,acc=0,last=performance.now(),pvDirty=true;
function pv(){pvDirty=true}
function hex(c,a){return c}
function frame(t){
 const dt=Math.min(100,t-last);last=t;
 if(S.ledCount){const n=S.ledCount,st=Math.max(15,S.speedDelay);acc+=dt;
  while(acc>=st){acc-=st;pos=(pos+(S.dirMode==1?-1:1)+n)%n;if(pos===tgt%n){tgt=(pos+3+Math.floor(Math.random()*(n-6)))%n}}
  g.clearRect(0,0,380,380);const R=150,cx=190,cy=190;
  for(let i=0;i<n;i++){const a=-Math.PI/2+i*2*Math.PI/n,x=cx+R*Math.cos(a),y=cy+R*Math.sin(a);
   let col=S.cBg;let r=12;const d=((pos-i)*(S.dirMode==1?-1:1)+n)%n;
   if(d>=1&&d<=S.tailLen){col=S.cCursor;g.globalAlpha=.7*(1-d/(S.tailLen+1))}else g.globalAlpha=1;
   if(i===tgt%n){col=S.cTarget;r=14;if(S.targetPulse)g.globalAlpha=.55+.45*Math.sin(t/180)}
   if(i===pos){col=S.cCursor;r=16;g.shadowColor=col;g.shadowBlur=22}else g.shadowBlur=0;
   g.fillStyle=col;g.beginPath();g.arc(x,y,r,0,7);g.fill();
   if(col===S.cBg){g.strokeStyle='#2a3266';g.lineWidth=2;g.stroke()}}
  g.globalAlpha=1;g.shadowBlur=0}
 requestAnimationFrame(frame)}

$('#nav').addEventListener('wheel',e=>{e.currentTarget.scrollLeft+=e.deltaY});
load().then(()=>{poll();live();requestAnimationFrame(frame)}).catch(()=>{setTimeout(()=>location.reload(),2500)});
</script>
</body>
</html>
)rawliteral";
