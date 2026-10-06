"""Explicit cold/idle API fixtures to complement transaction benchmarks.

These are NOT worst-case costs. Hardware service boundaries are listed in output.
"""
import argparse,json,struct,hashlib
from pathlib import Path
from c_vs_asm_compare import Profile,serial
from c_vs_asm_suite import select_image
from storage_machine import StorageMachine
from iar_image import image_paths
from verification_usb_try import init_transport
from unicorn import UC_HOOK_MEM_WRITE

B=0x21000000;T=B+0x400;S=B+0x800
PLANS={
 'crc16_ffInit':(1024,65535),'fw_end':(T,7),
 'log_addControlEvent':(71,B,1),'log_addEvent':(B,),
 'log_addPhotoEvent':(123,),'log_addSystemEvent':(13,),
 'log_addSystemEventID':(13,B),'log_addUpdateEvent':(130,0),
 'log_checkPending':(),'log_flushPending':(),
 'log_getRecordsCount':(),'log_setTime':(1700000400,),
 'mem_erase_sector':(0x41000,),'mem_patch':(0x41000,B,32),
 'mem_patch2':(0x41000,B,16,B+16,16),'mem_recover':(),
 'mem_replace_pages':(0x41000,S|1,B),'mem_write':(0x41000,B,32),
 'photo_cleanup_active':(),'photo_confirmed_offset':(0x200000,64),'photo_recover':(),
 'rs485_init':(),'rs485_set_config':(B,), 'server_rx_process':(),
 'sj_io_erase':(0x41000,),'sj_io_read':(0x41000,B,32),'sj_io_service':(),
 'sj_io_write':(0x41000,B,32),'sj_recover':(),'sj_replace_pages':(0x41000,S|1,B),
 'sj_span':(0x41000,32),'spi_init':(),'spi_initCS':(),'spi_read':(B,4),
 'spi_read_byte':(),'spi_send':(B,4),'spi_send_byte':(0xa5,),
 'timers_user_handle':(),'usb_bus_error_seen':(),'usb_clear_buffer':(),
 'usb_data_reading':(B,0),'usb_datareceived':(B,4),'usb_diag_fill':(B,),
 'usb_diag_fill_addr':(B,),'usb_diag_fill_errors':(B,),'usb_diag_fill_regs':(B,),
 'usb_get_state':(),'usb_in_done':(),'usb_init':(),'usb_request_recovery':(),
 'usb_send':(T,4),'usb_sendstr':(T,),'usb_sof_seen':(),'usb_timer1ms':(),
 'WG_readedID':(123456,),'user_addIdToTempBuffer':(B,2),
 'user_calcCRC16InFlash':(0x41000,64),'user_checkFirmware':(0x41000,4,0),
 'user_checkIdTimeEnd':(),'user_checkShedule':(0,),'user_clearMemBlock':(8,),
 'user_clearTempIdBuffer':(),'user_comm_balance':(123,), 'user_comm_boot':(),
 'user_comm_busy':(),'user_comm_link':(0,0),'user_comm_local_time':(),
 'user_comm_low_signal':(5,),'user_comm_rx':(B,0),'user_comm_rx_done':(0,0),
 'user_comm_server':(0,),'user_comm_sms':(T,T),
 'user_comm_start':('comm_settings','handlers',0),
 'user_comm_ussd':(),'user_comm_volte':(), 'user_dataconnection':(0,B,0),
 'user_debug':(T,), 'user_factoryReset':(),'user_getNumbersCount':(),
 'user_getPhotoCount':(),'user_getTempIDRecordsCount':(),'user_getweakday':(1700000400,),
 'user_incomingcall':(T,1),'user_indication':(),'user_init':(),'user_initvariables':(),
 'user_last10digitsnumber':(T,),'user_readconfig':(),'user_readinputs':(),
 'user_readsavedinformation':(),'user_send_answer_comand':(17,0,0,7),
 'user_send_anyData':(7,B,4,0,7),'user_send_configToConfigurator':(0,),
 'user_send_identPackToConfigurator':(0,),'user_send_readedIDtoServer':(B,0),
 'user_send_registrationData':(0,),'user_send_smsToServer':(T,),
 'user_send_statusPack':(0,),'user_send_statusPackSecond':(0,7),
 'user_send_statusPackToConfigurator':(0,),'user_send_statusUpdateFW':(19,0,0,7),
 'user_send_timeshtamp':(0,1700000400,1234,0,7),'user_sendsms':(T,T),
 'user_setdefault':(),'user_smsreceived':(T,T),'user_updateID':(B,),
'user_writeConfigAndDevice':(),
 'w25_erase_sector':(0x41000,),'w25_read_devID':(),'w25_write_data':(0x41000,B,0),
}
BOUNDARIES=('wdt_reload','timers_pause','Set_System','Set_USBClock','USB_Interrupts_Config','USB_Init')

class Probe(StorageMachine):
    def __init__(self):
        super().__init__();self.wires=[];self.u.mem_map(0x1ffff000,0x1000)
        self.u.mem_write(B,b'abcdefghij'+bytes(1014));self.u.mem_write(T,b'79991234567\0')
        # Valid callback address for page replacement: refuse a page without data.
        self.stubs[S]=lambda:self.ret(0)
        for n in BOUNDARIES:
            if n in self.symbols:self.stubs[self.addr(n)]=self.ret
        self.stubs[self.addr('usb_send')]=self.send
        self.stubs[self.addr('usb_try_send')]=self.send
        self.u.mem_write(self.addr('mem'),struct.pack('<IIII',0x1000,0x19000,0x1a000,0x19000))
        self.u.mem_write(self.addr('mem')+6*8,struct.pack('<II',0x1cd400,4096))
        self.write(0x40013000,0x40,2);self.write(0x40013008,3,2)
        self.write(0x40013800,0xc0,4);self.write(0x40004400,0xc0,4)
        self.write(self.device+self.d['datetime'],1700000400)
        init_transport(self,inp=0,out=0,epoch=0)
    def read(self):
        a,p,n,_=self.args();assert 0<=n<=4096 and a+n<=len(self.flash),(a,n)
        self.read_budget+=n;self.u.mem_write(p,bytes(self.flash[a:a+n]));self.ret(1)
    def send(self):
        p,n,_,_=self.args();assert n<=2048;self.wires.append(bytes(self.u.mem_read(p,n)));self.ret(1)

    def spi_nor(self):
        wire=dict(cmd=0,address=0,count=0,wel=0)
        def cs(u,access,a,n,value,_):
            if a==0x40010c14 and value==4096:wire.update(cmd=0,address=0,count=0)
            if a==0x40010c10 and value==4096:
                if wire['cmd']==6:wire['wel']=2
                elif wire['cmd']==4:wire['wel']=0
                elif wire['cmd']==0x20:
                    assert wire['wel'];a=wire['address'];self.flash[a:a+4096]=b'\xff'*4096;wire['wel']=0
        def byte():
            value=self.args()[0]&255
            if not wire['cmd']:wire['cmd']=value
            elif wire['count']<3:wire['address']=(wire['address']<<8)|value;wire['count']+=1
            self.ret()
        def read():
            p,n,_,_=self.args()
            if wire['cmd']==0x90:self.u.mem_write(p,b'\xc2\x17'[:n])
            else:self.u.mem_write(p,bytes(self.flash[wire['address']:wire['address']+n]));wire['address']+=n
            self.ret()
        self.u.hook_add(UC_HOOK_MEM_WRITE,cs,begin=0x40010c10,end=0x40010c14)
        for name,fn in (('spi_send_byte',byte),('spi_read_byte',lambda:self.ret(wire['wel'])),('spi_read',read)):
            self.stubs[self.addr(name)]=fn

def run(only=None):
    totals={};out={}
    for name,args in PLANS.items():
        if only and name not in only:continue
        local={};m=Probe();m.stubs.pop(m.addr(name),None);p=Profile(m,local)
        if name in ('user_factoryReset','user_init','user_initvariables','w25_erase_sector','w25_read_devID'):m.spi_nor()
        args=tuple(m.addr(a) if isinstance(a,str) else a for a in args)
        if len(args)>4:
            m.top-=32
            for i,value in enumerate(args[4:]):m.write(m.top+i*4,value)
        try:
            value=p.run(lambda:m.call(name,*args[:4],budget=30000000 if name=='user_init' else 3000000))
            out[name]={'status':'PASS','cost':p.last,'return_r0':value,'wire':serial(m.wires),
                       'nor_sha256':hashlib.sha256(m.flash).hexdigest(),'measured_functions':local}
            from c_vs_asm_report import merge_functions
            merge_functions(totals,local)
        except Exception as exc:out[name]={'status':'UNMEASURED','error':type(exc).__name__+': '+str(exc)}
        print(out[name]['status'],name,flush=True)
    return {'functions':totals,'fixtures':out,'hardware_boundaries':BOUNDARIES,
            'scope':'Cold/idle API inputs and erased NOR; no worst-case or peripheral timing claim',
            'elf_sha256':hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--variant',choices=('ASM','C','C_SPEED'),required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--only',action='append',choices=tuple(PLANS))
    a=p.parse_args();select_image(a.variant);result=run(a.only);result['variant']=a.variant
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
