#!/usr/bin/env python3
"""Append four diagnostic groups to the exact frozen coopfix77 full runner."""
import os
import sys
import test_coopfix77_local as prior
inherited = prior.inherited
INHERITED_TASKS = inherited.tasks
DIAGNOSTIC_TASKS = [
    ('coop-radio-recovery-regression', [['python3','tools/test_coop_radio_recovery_regression.py','--mode','both','--output-dir','build/coop-radio-recovery']]),
    ('coop-radio-device-diagnostics', [['python3','tools/test_coop_radio_diagnostics_device.py','--mode','both','--output-dir','build/coop-radio-device']]),
    ('coop-radio-runtime-ui', [['python3','tools/test_coop_radio_runtime_ui.py','--mode','both','--output-dir','build/coop-radio-runtime-ui']]),
    ('coop-radio-diagnostic-render', [['python3','tools/capture_coop_radio_diagnostics.py','--mode','both','--art','cartao/RPGPOKET/artes.pak','--out','build/coop-radio-render']]),
]
def tasks(focused):
    assert focused is False, 'Diagnostic qualification never accepts a focused plan'
    plan = prior.tasks(False)
    assert len(plan) == 152 and len(dict(plan)) == 152, 'Historical coopfix77 plan changed'
    assert not set(dict(plan)) & set(dict(DIAGNOSTIC_TASKS))
    return plan + DIAGNOSTIC_TASKS

def main():
    assert '--focused' not in sys.argv, 'Diagnostic qualification requires all 157 groups'
    assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1', 'Diagnostic qualification requires LeakSanitizer'
    inherited.tasks = tasks
    try:
        inherited.main()
    finally:
        inherited.tasks = INHERITED_TASKS
if __name__=='__main__':main()
