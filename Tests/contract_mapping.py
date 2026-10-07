"""Прежние host-группы и проверяющие их живые ELF-сценарии."""
REPLACEMENTS={
    'countends':'stage5 countends','logcommit':'stage5 logcommit',
    'sync':'sync entry',
    'access':'access','bootjournal':'config_power','crcblocks':'service',
    'flashcmds':'clear','fwstart':'firmware','idindex':'index',
    'idsync':'table','log':'upload','logupload':'upload','photo':'photo',
    'rs485cfg':'photo_edges',
    'sectorjournal':'table_power','serverrx':'commserver','storage':'storage_boundary',
    'utcday':'access','uploadretry':'upload',
    'cfgstrings':'protocol config', 'cmdlengths':'protocol lengths', 'commands':'protocol text',
    'fwblockbounds':'fw_blocks', 'crc':'stage3 input guards', 'crcff':'service', 'idtext':'logic ELF',
    'persistence':'config', 'rules':'fw_bridge', 'recovery':'stage4 USB', 'rx':'stage4 USB', 'w25':'index',
    'async config':'config','async callers':'owners',
    'async events':'event_append','async FW':'fw_blocks',
}
