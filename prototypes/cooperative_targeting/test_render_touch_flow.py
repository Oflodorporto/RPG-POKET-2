#!/usr/bin/env python3
"""Read-only renderer/trace QA. Does not execute or change firmware."""
import copy
import json
import re
from PIL import Image, ImageChops
import render_touch_flow as r


def run():
    out = r.ROOT / 'output/coop-ui-targeting'
    rows = [json.loads(line) for line in (out/'ui-trace.jsonl').read_text().splitlines()]
    header = r.Path(__file__).with_name('CoopTouchUi.h').read_text()
    for name, expected in [('kButtons', r.BUTTONS), ('kEnemyHits', r.ENEMY_HITS), ('kBossHit', [r.BOSS_HIT])]:
        body = re.search(r'constexpr Rect '+name+r'[^=]*=(.*?);', header).group(1)
        actual = [tuple(map(int, match)) for match in re.findall(r'\{(\d+),(\d+),(\d+),(\d+)\}', body)]
        assert actual == expected, (name, actual, expected)
    originals = copy.deepcopy(rows)
    images = [r.render(row) for row in rows]
    assert rows == originals
    for row, im in zip(rows, images):
        assert im.size == (320, 240)
        for slot in row['eligible_slots']:
            a = row['actors'][slot]
            x, y, w, h = r.BOSS_HIT if a['kind'] == 3 else r.ENEMY_HITS[slot-3]
            border = [(px, py) for px in range(x, x+w) for py in (y, y+h-1)]
            border += [(px, py) for py in range(y, y+h) for px in (x, x+w-1)]
            assert all(im.getpixel(p) == r.TARGET for p in border), ('incomplete target outline', row['step'], slot)
    pending = [i for i, row in enumerate(rows) if row['ui_mode'] == 'pending']
    focus = next(i for i, row in enumerate(rows) if row['ui_mode'] == 'target' and row['selected_action'] == 4)
    ability = next(i for i, row in enumerate(rows) if row['ui_mode'] == 'ability')
    cancel = next(i for i, row in enumerate(rows) if row['step'].startswith('Cancelado:'))
    confirmed = next(i for i, row in enumerate(rows) if row['step'] == 'Ataque confirmado pelo host')
    choose_ability = next(i for i, row in enumerate(rows) if row['step'] == 'J1: escolha uma habilidade')
    assert ImageChops.difference(images[pending[0]], images[pending[1]]).getbbox() is None, 'Unconfirmed host resolution changed the client view'
    assert rows[pending[0]]['actors'] == rows[pending[1]]['actors'] == rows[pending[0]-1]['actors']
    assert len({row['state_hash'] for row in rows[choose_ability:cancel+1]}) == 1
    assert rows[confirmed]['actors'][4]['hp'] == 0 and 4 not in rows[focus]['eligible_slots']
    assert r.controls(rows[pending[0]])[2] == [False]*4
    assert r.controls(rows[focus])[3] == [True, False, False, False]
    assert r.controls(rows[focus])[0] == ['FOCO*', '', '', 'CANCELAR']
    # Markers belong to pre-transition down states, on a visibly enabled target.
    marker_count = 0
    for row in rows:
        if row['click'] is None:
            continue
        marker_count += 1
        x, y = row['click']
        labels, _, enabled, _ = r.controls(row)
        hits = [i for i, (bx, by, bw, bh) in enumerate(r.BUTTONS) if bx <= x < bx+bw and by <= y < by+bh]
        if hits:
            assert len(hits) == 1 and labels[hits[0]] and enabled[hits[0]], ('marker over blank or disabled button', row['step'])
        else:
            assert any((lambda b: b[0] <= x < b[0]+b[2] and b[1] <= y < b[1]+b[3])(r.BOSS_HIT if row['actors'][slot]['kind'] == 3 else r.ENEMY_HITS[slot-3]) for slot in row['eligible_slots'])
    assert marker_count == 4
    assert rows[ability]['click'] is None and rows[cancel]['click'] is None
    # Additional renderer-only fixtures cover a submenu absent from this demo.
    item_fixture = copy.deepcopy(rows[choose_ability])
    item_fixture.update(ui_mode='items', eligible_slots=[])
    r.render(item_fixture)
    assert r.controls(item_fixture)[0] == ['VIDA', 'MANA', '', 'CANCELAR']
    ability_fixture = copy.deepcopy(rows[ability])
    ability_fixture['actors'][ability_fixture['local']]['mp'] = 0
    r.render(ability_fixture)
    assert r.controls(ability_fixture)[2][0] is False
    dead_fixture = copy.deepcopy(rows[focus])
    dead_fixture['eligible_slots'].append(4)
    try:
        r.validate_row(dead_fixture)
    except AssertionError:
        pass
    else:
        raise AssertionError('Renderer accepted dead enemy eligibility')
    for name in ('target', 'boss'):
        native = Image.open(out/f'{name}-native.png').convert('RGB')
        enlarged = Image.open(out/f'{name}-2x.png').convert('RGB')
        expected = native.resize((640, 480), Image.Resampling.NEAREST)
        assert ImageChops.difference(enlarged, expected).getbbox() is None
    manifest = json.loads((out/'render-manifest.json').read_text())
    for record in manifest['outputs']:
        assert r.sha(r.ROOT/record['path']) == record['sha256'], record['path']
    for record in manifest['source_files']:
        path = r.Path(record['path'])
        if not path.is_absolute():
            path = r.ROOT/path
        assert r.sha(path) == record['sha256'], record['path']
    gif = Image.open(out/'coop-touch-flow-local.gif')
    gif_indices = [i for i, row in enumerate(rows) if row['wave'] != 3]
    assert gif.n_frames == len(gif_indices) == 13
    total = 0
    for frame in range(gif.n_frames):
        gif.seek(frame)
        assert gif.size == (680, 650)
        total += gif.info['duration']
    assert total == 14400
    assert manifest['gif']['trace_indices'] == gif_indices
    result = {'result': 'PASS', 'cpp_renderer_geometry_match': True, 'all_target_outline_pixels_exact': True, 'unconfirmed_client_pixels_unchanged': True, 'selection_and_cancel_state_hash_unchanged': True, 'dead_enemy_rejected': True, 'all_four_touch_markers_on_enabled_visible_controls_or_live_targets': True, 'posttransition_markers_absent': True, 'items_and_zero_mp_fixtures': True, 'two_x_pixels_exact': True, 'manifest_hashes_verified': True, 'gif_frames': gif.n_frames, 'gif_duration_ms': total, 'boss_is_separate_static_fixture': True}
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    run()
