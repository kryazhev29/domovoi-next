"""Регрессии разблокировки: отдельный IAR ELF в TEMP и отказ assertion."""
from test_flash_mutations import run as run_mutations


def run():
    source = 'User/c/user_access.c'
    mutations = {
        'immediate-break': ('pulse_delay', [(source,
            'state = !next || unblock_wait;',
            'state = !next;')]),
        'no-level-restore': ('level_restore', [(source,
            'else if (!next && (unblock_level & (1u << i))) state = unblock_level >> 2;',
            'else if (!next && (unblock_level & (1u << i))) continue;')]),
    }
    mutations.update({
        'restore-opens': ('restart_and_commands', [(source,
            'unblock_pending |= restore ? 2u : 1u;',
            'unblock_pending |= 1u; (void)restore;')]),
        'gate-no-debounce': ('gate_sensor', [(source,
            'if (closed != (unblock_gate & 1u)) unblock_gate ^= 1u;',
            'if (0) unblock_gate ^= 1u;')]),
        'led-from-unblock': ('manual', [('User/c/user_main.c',
            '(device.status.stateOut2 && !config.rele[1].useInBlock)){',
            '(device.status.stateOut2)){')]),
        'gate-repeats': ('gate_sensor', [(source,
            'unblock_active && !(unblock_gate & 2u)',
            'unblock_active')]),
    })
    return run_mutations(mutations, 'test_unblock', 'unblock-delay-mutations')
