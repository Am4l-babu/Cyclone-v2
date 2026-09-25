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
    <div><span class="chip" id="mode">idle</span></div>
    <div class="big" id="time">--</div>
    <div class="bar"><i id="tbar"></i></div>
    <div class="grid3">
      <div class="kv"><b id="score">0</b><span>Score</span></div>
      <div class="kv"><b id="best">0</b><span>Best</span></div>
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

<script>
const $=s=>document.querySelector(s);
let S={},L={},M={},slots=[],dirty={},timer,ip='';

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
['select','dirMode','Direction','','dir']],
Look:[
['h','Running cursor'],
['color','cCursor','Cursor colour'],
['color','cTarget','Target colour'],
['color','cBg','Background colour'],
['range','tailLen','Comet tail length','LEDs'],
['toggle','targetPulse','Pulsing target'],
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
Presets:[['presets']],
System:[['system']]
};

// ---------- helpers ----------
function toast(t){const e=$('#toast');e.textContent=t;e.classList.add('show');clearTimeout(toast.t);toast.t=setTimeout(()=>e.classList.remove('show'),1800)}
function conn(ok){$('#conn').className='dot'+(ok?' ok':'');$('#connTxt').textContent=ok?'connected':'offline'}
async function api(p,o){try{const r=await fetch(p,o);const j=await r.json();conn(true);if(!r.ok)throw j;return j}catch(e){if(e&&e.error){toast(e.error)}else conn(false);throw e}}
function set(k,v){S[k]=v;dirty[k]=v;$('#saved').textContent='saving…';clearTimeout(timer);timer=setTimeout(flush,250)}
async function flush(){const b=new URLSearchParams(dirty);dirty={};try{await api('/api/set',{method:'POST',body:b})}catch(e){}}
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
 const rh=mk('button','no sm','Reset high score');rh.onclick=async()=>{if(confirm('Reset the high score?')){await api('/api/resethigh');toast('High score reset')}};
 const fr=mk('button','no sm','Factory reset');fr.onclick=async()=>{if(confirm('Restore ALL settings to defaults?')){await api('/api/factory');await load();toast('Defaults restored')}};
 const rb=mk('button','ghost sm','Reboot');rb.onclick=async()=>{if(confirm('Reboot the device?')){try{await api('/api/reboot')}catch(e){}toast('Rebooting…')}};
 d.appendChild(rh);d.appendChild(fr);d.appendChild(rb);box.appendChild(d)}

let cur='Game';
function build(){
 const nav=$('#nav'),tabs=$('#tabs');nav.innerHTML='';tabs.innerHTML='';
 Object.keys(TABS).forEach(n=>{
  const b=mk('button',n===cur?'on':'',n);b.onclick=()=>{cur=n;build()};nav.appendChild(b);
  const c=mk('div','card tab'+(n===cur?' on':''));
  TABS[n].forEach(sp=>{if(sp[0]==='presets')presets(c);else if(sp[0]==='system')system(c);else c.appendChild(control(sp))});
  tabs.appendChild(c)});
 refresh()}
function refresh(){
 document.querySelectorAll('[data-k]').forEach(e=>{const v=S[e.dataset.k];if(v===undefined)return;if(e.type==='checkbox')e.checked=!!v;else e.value=v});
 document.querySelectorAll('[data-o]').forEach(o=>{const i=document.querySelector('input[data-k="'+o.dataset.o+'"]');if(!i)return;const u=(TABS.Game.concat(TABS.Look,TABS.Effects,TABS.Sound).find(s=>s[1]===o.dataset.o)||[])[3];o.textContent=i.value+(u?' '+u:'')});
 pv()}
async function load(){const d=await api('/api/settings');S=d.v;L=d.limits;M=d.meta;slots=d.slots;ip=d.ip;build()}

// ---------- live status ----------
function fmt(s){return s>=100?Math.floor(s/60)+':'+String(s%60).padStart(2,'0'):s+'s'}
function draw(j){
 const m=$('#mode');m.textContent=j.mode;m.className='chip '+j.mode;
 $('#time').textContent=fmt(j.timeLeft);$('#score').textContent=j.score;$('#best').textContent=j.best;$('#level').textContent=j.level;
 $('#tbar').style.width=Math.min(100,100*j.timeLeft/Math.max(1,j.round))+'%';
 $('#saved').textContent=j.saved?'settings saved ✓':'saving…';
 const i=$('#info');if(i)i.innerHTML='IP: '+ip+'<br>Clients: '+j.clients+'<br>Free heap: '+j.heap+' B<br>Uptime: '+fmt(j.up)}
async function poll(){try{draw(await api('/api/state'))}catch(e){}}

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
load().then(()=>{poll();setInterval(poll,700);requestAnimationFrame(frame)}).catch(()=>{setTimeout(()=>location.reload(),2500)});
</script>
</body>
</html>
)rawliteral";
