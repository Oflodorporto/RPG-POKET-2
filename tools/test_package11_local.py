#!/usr/bin/env python3
"""Resumable host-only package11 validation; never invokes release/build/publish gates.

Checkpoints are bound to every non-ignored source file, compiler and sanitizer
configuration. A resumed frozen run skips only targets recorded as successful.
Source changes require a new --run-id, so stale passing evidence is never reused.
"""
from pathlib import Path
import argparse, concurrent.futures, contextlib, datetime, hashlib, json, os, re, subprocess, threading, time
ROOT=Path(__file__).resolve().parents[1]

def inventory():
    raw=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT)
    paths=sorted(set(x.decode() for x in raw.split(b'\0') if x))
    files={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths if (ROOT/p).is_file()}
    payload={'files':files,'compiler':subprocess.check_output(['g++','--version'],text=True).splitlines()[0],
             'detect_leaks':os.environ.get('RPG_DETECT_LEAKS','0'),'assertions':True,'ubsan_halt':True}
    payload['sha256']=hashlib.sha256(json.dumps(payload,sort_keys=True).encode()).hexdigest()
    return payload

def cpp_commands(name,modes):
    extras=[]
    if name in {'clock77_render','clock_cycle7_render','enemy_render','scale77_render','d20_camp77_render','camp_identity77_render','ruins7_noalloc','pvp_presentation_render','pvp_review_render'}:
        extras+=['-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc']
    if name=='clock77_render':extras+=['-DRPG_TEST_CLOCK_ALLOCATIONS']
    if name=='bulk_commerce_render':extras+=['-DRPG_TEST_COMMERCE_ALLOCATIONS','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc']
    if name in {'ota_recovery','ota_art_readback'}:extras+=['tests/update_fake/cJSON.c','-Itests/update_fake','-Ifirmware/RPG_POKET_2','-DRPG_TEST_VERIFY_DETAIL','-Wno-deprecated-declarations','-lcrypto']
    if name in {'update_adapter','enemy_ota','ota_sd_diagnostics'}:extras+=['tests/update_fake/cJSON.c','-Itests/update_fake']
    if name in {'settings_adapter','boot_art_diagnostics'}:extras+=['-Itests/settings_fake']
    if name=='shared_spi_startup':extras+=['-Itests/shared_spi_fake']
    if name=='sd_adapter':extras+=['-Itests/sd_fake']
    if name=='fatigue76_club_radio':extras+=['firmware/RPG_POKET_2/ClubProtocol.cpp','firmware/RPG_POKET_2/ClubDuel.cpp','-Itests/club_fake']
    if name in {'club_duel','club_protocol'}:extras+=['firmware/RPG_POKET_2/ClubProtocol.cpp','firmware/RPG_POKET_2/ClubDuel.cpp','-Ifirmware/RPG_POKET_2']
    if name.startswith('pvp_'):extras+=['firmware/RPG_POKET_2/ClubProtocol.cpp','firmware/RPG_POKET_2/ClubDuel.cpp','-Ifirmware/RPG_POKET_2','-Itests/club_fake']
    if name=='fatigue7':extras+=['firmware/RPG_POKET_2/ClubDuel.cpp']
    for mode in modes:
        flags=['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
        binary=f'build/package11-binaries/{name}_{mode}'
        yield ['g++','-std=c++17','-UNDEBUG',*flags,f'tests/{name}.cpp',*extras,'-o',binary]
        args=[]
        if name=='bulk_commerce_render':args=['build/bulk-shop/'+('screens' if mode=='normal' else 'sanitized')]
        elif name=='pvp_presentation_render':args=['cartao/RPGPOKET/artes.pak','build/pvp-presentation/'+('screens' if mode=='normal' else 'sanitized')]
        elif name=='pvp_review_render':args=['cartao/RPGPOKET/artes.pak','build/pvp-review/screens-'+mode]
        elif name=='pvp_review_equivalence':
            yield ['python3','tools/check_pvp_equivalence.py',binary]
            continue
        elif name=='procedural6':args=['100000' if mode=='normal' else '10000']
        elif name=='narrative8_events':args=['build/narrative8-events']
        elif name=='narrative8_render':args=['build/narrative8-render/aggregate-'+mode,'cartao/RPGPOKET/artes.pak']
        elif name=='hero77_controller':args=['build/hero77-coverage.csv']
        elif name=='commerce75':args=['build/commerce75-economy.csv']
        elif name=='balance74':args=['build/balance74-rewards.csv']
        elif name=='progression_journey':args=['build/progressao.csv']
        elif name=='ruins7_render':args=['build/ruins7']
        elif name in {'clock77_render','clock_cycle7_render'}:args=['build/clock-cycle7']
        elif name=='ruins_bag77_render':args=['build/ruins-bag77']
        elif name=='package6_integration_render':args=['build/screens/package6-integration']
        elif name=='procedural6_render':args=['build/screens/procedural6']
        elif name in {'fatigue7_render','fatigue76_render','combat_trace_render'}:args=['build/fatigue7/screens']
        elif name=='hero77_render':args=['build/screens/hero77']
        elif name=='visual77_render':args=['build/screens/visual77']
        elif name.endswith('_render') or name=='render':args=['build/screens']
        if name in {'camp_identity77_render','d20_camp77_render'}:args=[]
        yield [binary,*args]

def tasks(focused):
    names=['narrative8_controller','narrative8_events','narrative8_render','package7_integration_controller','ruins7_controller','camp_meal7_controller','camp_meal7_errors','clock_cycle7_logic','clock_cycle7_render','fatigue7','fatigue7_crossings','fatigue7_controller','fatigue7_render']
    names += 'pvp_reliability_protocol pvp_reliability_radio pvp_reliability_races pvp_reliability_closing pvp_reliability_persistence pvp_reliability_ordering pvp_reliability_save30 pvp_appearance_protocol pvp_presentation_render pvp_review_integration pvp_review_render pvp_review_ordering pvp_review_equivalence'.split()
    normal=set(names);san=set(names)
    if not focused:
        native=(ROOT/'tools/test_native.sh').read_text()
        loops=re.findall(r'for name in ([^;]+); do',native)
        normal.update(loops[0].split());san.update(loops[1].split())
        added='procedural6 procedural6_persistence procedural6_controller procedural6_render dungeon_save island_save event_expedition combat_trace_semantics combat_trace_controller combat_trace_render package6_integration_controller package6_integration_render d20_camp77_render d20_camp77_controller camp_identity77_render ruins_bag77_controller ruins_bag77_render ruins7_noalloc ruins7_render class_powers martial_powers club_duel club_protocol camp_loot daily_travel29 daily_travel29_combat ota_recovery'
        normal.update(added.split());san.update(added.split())
    result=[]
    for name in sorted(normal):result.append((name,list(cpp_commands(name,['normal','sanitizer'] if name in san else ['normal']))))
    result.extend([('merged-cadence',[['python3','tools/test_package7_cadence.py']]),('clock-real-adapters',[['python3','tools/test_clock_cycle7.py']])])
    if not focused:
        result.extend([
          ('enemy-transition-normal',[['python3','tools/test_enemy_transition.py']]),
          ('enemy-transition-sanitizer',[['python3','tools/test_enemy_transition.py','--sanitizers']]),
          ('save29-30-review-normal',[['python3','tools/test_package6_save_review.py']]),
          ('save29-30-review-sanitizer',[['python3','tools/test_package6_save_review.py','--sanitizers']]),
          ('combat-differential-normal',[['python3','tools/test_combat_trace.py','--baseline','tests/fixtures/package6-procedural-only','--save-format','30','--seeds','64','--report','build/package7-differential.json']]),
          ('combat-differential-sanitizer',[['python3','tools/test_combat_trace.py','--baseline','tests/fixtures/package6-procedural-only','--save-format','30','--seeds','16','--sanitizers','--report','build/package7-differential-sanitizer.json']]),
        ])
    # Keep all 143 inherited checkpoints; append only candidate-specific coverage.
    for name in ('bulk_commerce','bulk_commerce_controller','bulk_commerce_render','ration_limit10'):
        result.append((name,list(cpp_commands(name,['normal','sanitizer']))))
    result.extend([
        ('ration-limit10-oldreader-normal',[['python3','tools/test_ration_limit10_oldreader.py']]),
        ('ration-limit10-oldreader-sanitizer',[['python3','tools/test_ration_limit10_oldreader.py','--sanitizers']]),
    ])
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--focused',action='store_true');p.add_argument('--run-id',required=True);p.add_argument('--jobs',type=int,default=2);a=p.parse_args()
    assert re.fullmatch(r'[a-zA-Z0-9_.-]+',a.run_id) and 1<=a.jobs<=4
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        assert not a.focused, 'CI must run the complete inherited and commerce plan'
        assert os.environ.get('RPG_DETECT_LEAKS') == '1', 'LeakSanitizer is mandatory in CI'
    os.chdir(ROOT);env=dict(os.environ,ASAN_OPTIONS='detect_leaks='+os.environ.get('RPG_DETECT_LEAKS','0'),UBSAN_OPTIONS='halt_on_error=1',LSAN_OPTIONS='exitcode=23',NARRATIVE8_SKIP_IMAGES='1')
    for d in ['bulk-shop/screens','bulk-shop/sanitized','pvp-presentation/screens','pvp-presentation/sanitized','pvp-review/screens-normal','pvp-review/screens-sanitizer','narrative8-events','narrative8-render/aggregate-normal','narrative8-render/aggregate-sanitizer','package11-focused','package11-binaries','tests','screens','screens/fatigue76','screens/visual77','screens/hero77','screens/enemy77','screens/camp-identity77','screens/package6-integration','screens/procedural6','fatigue7/screens','ruins7','ruins-bag77','d20-camp77','clock-cycle7']:(ROOT/'build'/d).mkdir(parents=True,exist_ok=True)
    out=ROOT/'build/package11-validation'/a.run_id;out.mkdir(parents=True,exist_ok=True)
    manifest=inventory();frozen=out/'source-manifest.json'
    if frozen.exists():assert json.loads(frozen.read_text())==manifest,'Frozen input changed; create a new run-id instead of reusing evidence.'
    else:frozen.write_text(json.dumps(manifest,indent=2)+'\n')
    selected=tasks(a.focused);plan={'focused':a.focused,'tasks':selected,'source_sha256':manifest['sha256']};planpath=out/'plan.json'
    if planpath.exists():assert json.loads(planpath.read_text())==json.loads(json.dumps(plan)),'Run plan changed'
    else:planpath.write_text(json.dumps(plan,indent=2)+'\n')
    checkpoints=out/'checkpoints.json';state=json.loads(checkpoints.read_text()) if checkpoints.exists() else {};lock=threading.Lock();oldreader_lock=threading.Lock()
    def run(item):
        name,commands=item
        if state.get(name,{}).get('status')=='passed':
            recorded=state[name];log=out/(name+'.log')
            assert recorded.get('commands') == commands, 'Checkpoint commands changed: '+name
            assert log.is_file() and hashlib.sha256(log.read_bytes()).hexdigest() == recorded.get('log_sha256'), 'Checkpoint log missing or changed: '+name
            print('RESUME skip '+name,flush=True);return True
        start=time.time();entry={'status':'running','started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commands':commands}
        with lock:
            state[name]=entry;tmp=checkpoints.with_suffix('.tmp');tmp.write_text(json.dumps(state,indent=2)+'\n');tmp.replace(checkpoints)
        print('RUN '+name,flush=True)
        try:
            # Both frozen old-reader modes stage the same exact header closure.
            # Serialize their original tools so a compile never sees a rewrite.
            with oldreader_lock if name.startswith('ration-limit10-oldreader-') else contextlib.nullcontext():
                with (out/(name+'.log')).open('w') as log:
                    for command in commands:
                        log.write('$ '+' '.join(command)+'\n');log.flush();subprocess.run(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
            entry.update(status='passed',seconds=round(time.time()-start,2),log_sha256=hashlib.sha256((out/(name+'.log')).read_bytes()).hexdigest())
        except Exception as e:
            entry.update(status='failed',seconds=round(time.time()-start,2),error=str(e))
            log_path=out/(name+'.log')
            if log_path.exists():print('FAILED LOG '+name+'\n'+log_path.read_text(errors='replace')[-8000:],flush=True)
        with lock:
            state[name]=entry;tmp=checkpoints.with_suffix('.tmp');tmp.write_text(json.dumps(state,indent=2)+'\n');tmp.replace(checkpoints)
        print(entry['status'].upper()+' '+name,flush=True);return entry['status']=='passed'
    prep=[['python3','tools/check_controller_snapshot.py'],['python3','tools/unpack_art.py'],['python3','tools/unpack_art.py','--baseline-only','--output','build/enemy_transition/old.pak']]
    if not a.focused:prep += [[ 'python3',x,*args] for x,args in [('tools/prepare_clock_art.py',['--check']),('tools/prepare_scene_art.py',['--check']),('tests/scene_asset_catalog.py',[]),('tools/prepare_enemy_art.py',['--check','--extract-reference','build/enemy_art_source']),('tests/enemy_asset_catalog.py',[]),('tools/prepare_d20_mesh.py',['--check']),('tools/prepare_tent_marker.py',['--check']),('assets/reusable/effects01/check_library.py',[]),('assets/reusable/equipment01/check_library.py',[])]]
    if not run(('preparation',prep)):raise SystemExit('Preparation failed; inspect checkpoint log')
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:results=list(pool.map(run,selected))
    assert inventory()==manifest,'Source changed during validation; do not use these results to qualify the new source.'
    report={'source_sha256':manifest['sha256'],'all_passed':all(results),'tasks':len(selected)+1,'failed':[k for k,v in state.items() if v['status']!='passed'],'lsan':env['ASAN_OPTIONS'],'esp32_build':False,'publication':False}
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
    if not all(results):raise SystemExit(1)
if __name__=='__main__':main()
