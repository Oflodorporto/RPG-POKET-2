#!/usr/bin/env python3
"""Append eight radio suites to the exact 156-task historical plan, plus preparation."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
import os
import sys
import test_coopdiag77_local as prior
inherited = prior.inherited
INHERITED_TASKS = inherited.tasks
RADIO_SUITES = ('model','flow','device','races','export','runtime','render-on','render-off')
RADIO_TASKS = [
    ('coop-wifi-radio-'+suite, [
        ['python3','tools/test_wifi_radio_test.py','--suite',suite,'--out','build/wifi-radio-test/'+suite+'/normal'],
        ['python3','tools/test_wifi_radio_test.py','--suite',suite,'--out','build/wifi-radio-test/'+suite+'/sanitizers','--sanitizers'],
    ]) for suite in RADIO_SUITES
]
def tasks(focused):
    assert focused is False, 'Wi-Fi qualification never accepts a focused plan'
    plan = prior.tasks(False)
    assert len(plan) == 156 and len(dict(plan)) == 156, 'Historical coopdiag77 plan changed'
    assert not set(dict(plan)) & set(dict(RADIO_TASKS))
    return plan + RADIO_TASKS

def main():
    assert '--focused' not in sys.argv, 'Wi-Fi qualification requires all 165 groups'
    assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1', 'Wi-Fi qualification requires LeakSanitizer'
    inherited.tasks = tasks
    try:
        inherited.main()
    finally:
        inherited.tasks = INHERITED_TASKS
if __name__=='__main__':main()
