"""Две регрессии разблокировки: отдельный IAR ELF в TEMP и отказ assertion."""
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
    return run_mutations(mutations, 'test_unblock', 'unblock-delay-mutations')
