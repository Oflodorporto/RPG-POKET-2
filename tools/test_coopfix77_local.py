#!/usr/bin/env python3
"""Append one hotfix group to the exact frozen package12 runner; never publish.

All inherited commands, preparation, source/compiler/sanitizer binding and
checkpoint checks execute through the untouched historical runner. Evidence
therefore intentionally remains in build/package12-validation.
"""
import os
import sys
import test_package12_local as inherited

INHERITED_TASKS = inherited.tasks
HOTFIX_TASK = ('coop-touch-invite-terminal-regression', [
    ['python3', 'tools/test_coop_touch_queue_regression.py', '--mode', 'both', '--output-dir', 'build/coop-touch-queue'],
    ['python3', 'tools/test_coop_invite_countdown_regression.py', '--mode', 'both', '--output-dir', 'build/coop-invite-countdown'],
    ['python3', 'tools/check_coopfix77_invite_render.py'],
])

def tasks(focused):
    assert focused is False, 'Hotfix qualification never accepts a focused plan'
    plan = INHERITED_TASKS(False)
    assert len(plan) == 151 and len(dict(plan)) == 151, 'Historical package12 plan changed'
    assert HOTFIX_TASK[0] not in dict(plan)
    return plan + [HOTFIX_TASK]

def main():
    assert '--focused' not in sys.argv, 'Hotfix qualification requires all 153 groups'
    assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1', 'Hotfix requires LeakSanitizer'
    inherited.tasks = tasks
    try:
        inherited.main()
    finally:
        inherited.tasks = INHERITED_TASKS

if __name__ == '__main__':
    main()
