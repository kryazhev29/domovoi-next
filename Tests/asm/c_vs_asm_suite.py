"""Запуск тестов собранного IAR-кода; результаты сохраняются в папке сборок."""
import argparse
import contextlib
import importlib
import io
import json
import os
from pathlib import Path
import runpy
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Tests'))
import iar_irq_support

CASES = {
    'peripheral init': [('test_iar_peripheral_config','current')],
    'protocol lengths': [('test_protocol_guards','lengths')],
    'protocol config': [('test_protocol_guards','config_strings')],
    'protocol text': [('test_protocol_guards','text_commands')],
    'sched16': [('test_sched16', n) for n in ('protocol','crc_paths','access','migration')],
    'ids boot read': [('test_ids_boot_read','main')],
    'sched16_power': [('test_sched16','migration_power')],
    'stage5 countends': [('test_stage5_log','count_ends')],
    'stage5 logcommit': [('test_stage5_log','record_commit')],
    'stage5 2MB': [('test_stage5_capacity','run')],
    'stage5 log': [('test_stage5_log',n) for n in ('run','cursor_loss','migration','legacy_checkpoint','generation_bytes')],
    'stage5_log_power': [('test_stage5_log',n) for n in ('power','migration_power','erased_generation')],
    'stage5 CRC': [('test_stage5_crc', n) for n in ('run','real_nor')],
    'stage5_crc_power': [('test_stage5_crc', n) for n in ('power','background','cancellation','loader_recovery','failed_commit_recovery')],
    'stage5 ID': [('test_stage5_id', 'run')],
    'stage5 Wiegand': [('test_stage5_wiegand', 'run')],
    'stage4 USB': [('test_stage4_usb', 'run')],
    'stage4 FW CRC': [('test_stage4_fw_crc', 'run')],
    'sync entry': [('test_project_sync_entry','run')],
    'stage3 input guards': [('test_stage3_entry_guards','run')],
    'diagnostics config': [('test_diagnostics_config','run')],
    'stack canary': [('test_stack_canary','run')],
    'format ABI': [('test_user_format_runtime','run')],
    'workspace': [('test_work_memory', 'run')],
    'id_sync_fast': [('test_id_sync_fast', 'contracts'), ('test_id_sync_fast', 'mutations')],
    **{n:[('c_vs_asm_extra',n)] for n in ('event_append','fw_blocks','fw_bridge','usb_queue','owner_lifetimes')},
    'access': [('test_access_reads', n) for n in ('table_cells','holidays','find_id','access')],
    'config': [('test_memory_runtime', n) for n in ('allocator','cached_reads','config_ownership','config_faults')],
    'index': [('test_idindex_runtime', n) for n in ('test_normal','test_faults','test_cancel_restart','test_boundaries','test_real_w25')],
    'service': [('test_service_runtime', n) for n in ('wire_and_budgets','faults_and_lifetime','config_reply','config_cached_source','ff_affine','pending_timeout','real_w25','unrecovered_storage')],
    'upload': [('test_upload_runtime', n) for n in ('test_normal','test_parser','test_media_gate','test_gaps_and_faults','test_busy_empty_cancel','test_heap_ack')],
    'clear': [('test_idclear_runtime', n) for n in ('success_and_heap','sessions_cancel_faults','guards')],
    'expiry': [('test_expiry_runtime', n) for n in ('cancel_and_sessions','faults','full_scan_and_heap')],
    'firmware': [('test_fw_service_runtime', n) for n in ('normal','heap_reservation','boundaries_errors','io_faults','busy_cancel','real_spi','exact_failure_logs')],
    'photo': [('test_photo_runtime', n) for n in ('normal','uploads','errors')],
    'photo_edges': [('test_photo_edges', n) for n in ('cancellation','corrupt','failures','stream','transport_and_heap','ownership','full_cleanup','headers_and_space')],
    'table': [('test_table_runtime', n) for n in ('test_cancel_and_reconnect','test_faults','test_corrupt','test_interaction_and_heap')],
    'owners': [('test_owner_integration', n) for n in ('expiry_scheduler','clear_scheduler')],
    'usb_replies': [('test_owned_usb_replies','run')],
    'storage_boundary': [('test_storage_boundaries',None)],
    'config_power': [('test_memory_runtime','power_cuts')],
    'index_power': [('test_idindex_runtime','test_power')],
    'upload_power': [('test_upload_runtime','test_ack_power')],
    'clear_power': [('test_idclear_runtime','bulk_power')],
    'expiry_power': [('test_expiry_runtime','power')],
    'firmware_power': [('test_fw_service_runtime','power_cuts')],
    'photo_power': [('test_photo_power', n) for n in ('capture_cuts','acknowledgement_cuts','cleanup_cuts')],
    'table_power': [('test_table_runtime','test_power')],
}
CORE = ('peripheral init','protocol lengths','protocol config','protocol text','sched16','ids boot read','stage5 countends','stage5 logcommit','stage5 2MB','stage5 log','stage5 CRC','stage5 ID','stage5 Wiegand','stage4 FW CRC','stage4 USB','sync entry','stage3 input guards','diagnostics config','stack canary','format ABI','workspace','id_sync_fast','access','config','index','service','upload','clear','expiry','firmware','photo','photo_edges','table','owners','owner_lifetimes','usb_replies','storage_boundary')
POWER = tuple(n for n in CASES if n.endswith('_power'))


def select_image(variant, build_root=None):
    base = Path(build_root or os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir()) / 'domovoi-c-asm'))
    folder = base / (variant + '-' + os.environ.get('DOMOVOI_COMPARE_CONFIG','Release'))
    stem = 'DomovoiNext_C' if variant.startswith('C') else 'DomovoiNext_' + variant
    os.environ.update(DOMOVOI_ELF=str(folder/'ExePath/DomovoiNext.out'),
                      DOMOVOI_BIN=str(folder/'ExePath'/f'{stem}.bin'),
                      DOMOVOI_MAP=str(folder/'ListPath/DomovoiNext.map'))
    os.environ.pop('DOMOVOI_MODEL_FLASH_BYTES',None)
    os.environ.pop('DOMOVOI_MODEL_RAM_BYTES',None)
    return folder


def run_cases(cases):
    results = {}
    for case in dict.fromkeys(cases):
        start = time.monotonic()
        captured = io.StringIO()
        try:
            details = {}
            with contextlib.redirect_stdout(captured):
                for module, name in CASES[case]:
                    if name is None:
                        runpy.run_path(str(ROOT/'Tests/asm'/f'{module}.py'))
                    elif case == 'usb_replies':


                        details[name] = importlib.import_module(module).run(instruction_limit=None)
                    else:
                        details[name] = getattr(importlib.import_module(module), name)()
                if case == 'table':
                    m = importlib.import_module('test_table_runtime')
                    details['normal'] = [m.test_normal(cmd,ids,True) for cmd,ids in (
                        (13,[1]),(13,list(range(1,64))),(13,[8,9,128,129,1,129]),
                        (14,[8,9,64]),(15,[16,17,128]),(16,[1,2]))]
            results[case] = {'status':'PASS','seconds':round(time.monotonic()-start,3),'details':details}
            print(f'PASS ELF {case} {results[case]["seconds"]:.1f}s',flush=True)
        except Exception as exc:
            import traceback
            results[case] = {'status':'FAIL','seconds':round(time.monotonic()-start,3),
                             'error':str(exc),'traceback':traceback.format_exc()}
            print(f'FAIL ELF {case}: {exc}',flush=True)
            print(results[case]['traceback'][-1700:],flush=True)
    return results


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--variant',choices=('C','HYBRID'),required=True)
    p.add_argument('--build-root',type=Path)
    p.add_argument('--case',action='append',choices=tuple(CASES))
    p.add_argument('--power',action='store_true')
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    select_image(args.variant,args.build_root)
    cases=args.case or list(CORE)
    if args.power:cases=list(cases)+list(POWER)
    results=run_cases(cases)
    if args.output:args.output.write_text(json.dumps({'variant':args.variant,'cases':results},indent=2,default=str)+'\n',encoding='utf-8')
    return 0 if all(r['status']=='PASS' for r in results.values()) else 1

if __name__=='__main__':sys.exit(main())
