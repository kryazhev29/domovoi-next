"""Первая пачка сверки: мутации IAR-кода ловятся существующими сценариями ELF."""
from test_flash_mutations import run as run_mutations


def run():
    sms = 'User/c/user_sms.c'
    access = 'User/c/user_access.c'
    outs = 'User/outs.c'
    scenarios = {
        'D13-direct-output': ('sms_outputs', [(sms,
            'user_activateOut(out == 1, out == 2, CT_SERVER, NULL)',
            '(out_load(out - 1, OUT_ON, 0, 0, 0), true)')]),
        'D13-server-setting': ('sms_outputs', [(access,
            '(tp != CT_SERVER || config.rele[out].onCmdServer)', 'true')]),
        'D13-useInBlock': ('sms_outputs', [(access,
            'run[out] && !config.rele[out].useInBlock', 'run[out]')]),
        'D13-pause': ('sms_outputs', [(access,
            ' && !out_getmodestate(out)', '')]),
        'D13-unblock': ('sms_outputs', [(access,
            'if (unblock_active) return false;', 'if (false) return false;')]),
        'D13-level-toggle': ('sms_outputs', [(access,
            '(tp == CT_SERVER) ? OUT_ON : OUT_INV', 'OUT_INV')]),
        'D13-extra-event': ('sms_outputs', [(access,
            'if (id != NULL) log_addControlEvent(server_out_event, id, cnt);',
            'log_addControlEvent(server_out_event, id, cnt);')]),
        'D21-numeric-prefix': ('sms_format', [(sms,
            "    if ((arg[0] != '1' && arg[0] != '2') || arg[1] != 0) goto format_error;\n    out = arg[0] - '0';",
            "    out = 0;\n    while (*arg >= '0' && *arg <= '9') out = (uint8_t)(out * 10u + *arg++ - '0');\n    if (out != 1 && out != 2) goto format_error;")]),
        'D09-captured-access': ('capture_id', [(access,
            '    return;\n  }\n  user_checkIdAndControl(tb, CT_WIEGAND);',
            '  }\n  user_checkIdAndControl(tb, CT_WIEGAND);')]),
        'D09-other-source-access': ('capture_id', [(access,
            'if (device.timeToGetID) {', 'if (device.timeToGetID && device.sourceGetID == 1) {')]),
        'D14-ID-length': ('registration_write', [(sms,
            '!validFormat || strlen(arg) != 10u', '!validFormat')]),
        'D14-ID-digits': ('registration_write', [(sms,
            "if (arg[i] < '0' || arg[i] > '9') goto format_error;", 'if (false) goto format_error;')]),
        'D14-SMS-false-success': ('registration_write', [(sms,
            'if (!user_addIdToTempBuffer(arg, CT_OTHER)) return true;',
            'user_addIdToTempBuffer(arg, CT_OTHER);')]),
        'D14-autoreg-false-event': ('registration_write', [(access,
            'device.status.autoRegistration && user_addIdToTempBuffer(id, tp)',
            'device.status.autoRegistration && (user_addIdToTempBuffer(id, tp), true)')]),
        'D14-write-false-success': ('registration_write', [('User/c/user_ids.c',
            'if (!ok) return false;',
            'if (!ok) return true;')]),
        'D31-infinite-without-pause': ('repeats255', [(outs,
            '            out_period[i]--;', '            if (out_period[i] != 255) out_period[i]--;')]),
        'D31-infinite-with-pause': ('repeats255', [(outs,
            '          out_time[i] = out_timeactiv[i];\n          out_period[i]--;',
            '          out_time[i] = out_timeactiv[i];\n          if (out_period[i] != 255) out_period[i]--;')]),
    }
    results = run_mutations(scenarios, 'test_unblock', 'fixes1-mutations')
    protocol = {
        'D33-status-constant': ('rx_capacity', [('User/c/user_protocol.c',
            'tmp32 = user_comm_rx_capacity();', 'tmp32 = 2048;')]),
        'D33-answer-constant': ('rx_capacity', [('User/packet.c',
            'rt_put16(packet+15,user_comm_rx_capacity());', 'rt_put16(packet+15,2048);')]),
        'D02-TTL255': ('fw_request_ttl', [('User/c/user_protocol.c',
            'send_buffer[6] = 20;', 'send_buffer[6] = 255;')]),
    }
    results.update(run_mutations(protocol, 'test_protocol_guards', 'fixes1-mutations'))
    return results
