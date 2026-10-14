#!/usr/bin/env python3
"""Render a local 320x240 touch-flow trace, without importing the old renderer.

All combat values come from input snapshots. This renderer neither runs firmware
nor invents damage, ACKs, latency, radio traffic, or device frame-rate results.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import platform
import re
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[3]
HELPER = ROOT / 'coop-prototype-work/prototypes/cooperative_preview/extract_assets.py'
spec = importlib.util.spec_from_file_location('coop_frozen_art', HELPER)
art = importlib.util.module_from_spec(spec)
spec.loader.exec_module(art)  # Read-only helper; its contact-sheet main is not run.
FW, SRC = art.FW, art.SRC
FONT_SOURCE = FW / 'src/GFX/font/glcdfont.h'
font_body = FONT_SOURCE.read_text().split('font[]')[1].split('{', 1)[1].split('};', 1)[0]
FONT = bytes(int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{2})', font_body))
assert len(FONT) >= 1280

SIZE = (320, 240)
ARENA = (3, 42, 314, 106)
EVENT = (4, 150, 312, 14)
STATS = [(4, 167, 154, 29), (162, 167, 154, 29)]
BUTTONS = [(4, 199, 75, 30), (83, 199, 75, 30), (162, 199, 75, 30), (241, 199, 75, 30)]
HEROES = [(18, 54, 58, 64), (78, 76, 58, 64)]
ENEMIES = [(192, 51, 48, 40), (260, 75, 48, 40), (191, 97, 48, 40)]
BOSS = (212, 61, 84, 77)
ENEMY_HITS = [(185, 45, 61, 50), (252, 70, 63, 53), (185, 98, 61, 50)]
BOSS_HIT = (202, 50, 104, 98)
GOLD = (232, 193, 119)
WHITE = (246, 237, 210)
BLUE = (125, 197, 238)
GREEN = (122, 213, 126)
RED = (228, 107, 91)
MUTED = (178, 174, 151)
INK = (10, 23, 26)
DISABLED = (118, 132, 137)
TARGET = (123, 248, 197)
NAMES = {1: 'J1', 2: 'J2', 10: 'GOBLIN', 11: 'LOBO', 12: 'OSSOS', 13: 'JAVALI', 14: 'LOBO', 15: 'OSSOS', 16: 'GUARDIAO'}
CLASSES = ['MAGO', 'CAVALEIRO', 'GUERREIRO', 'BARBARO']
FRAME_PATH = SRC / 'assets/ruins7/ui-frame.png'
FRAME = Image.open(FRAME_PATH).convert('RGBA')
BG = art.indexed(59, 58).resize((240, 320), Image.Resampling.NEAREST).crop((0, 110, 240, 300))
PROFILES = []
for line in (FW / 'EnemyScale.h').read_text().split('constexpr Profile profiles')[1].split('};', 1)[0].splitlines():
    match = re.search(r'\{(\d+),(\d+),(\d+),(\d+),', line)
    if match:
        PROFILES.append(tuple(map(int, match.groups())))
assert len(PROFILES) == 29
SPRITES = {}
TEXT_AUDIT = []


def bounds_intersect(a, b):
    return max(a[0], b[0]) < min(a[0] + a[2], b[0] + b[2]) and max(a[1], b[1]) < min(a[1] + a[3], b[1] + b[3])


def rect(im, box, color):
    x, y, w, h = box
    if w > 0 and h > 0:
        ImageDraw.Draw(im).rectangle((x, y, x + w - 1, y + h - 1), fill=color)


def text(im, x, y, label, color=WHITE, center=None, region=None):
    width = len(label) * 6 - 1
    if center is not None:
        x = center - width // 2
    assert 0 <= x and 0 <= y and x + width <= im.width and y + 8 <= im.height, (label, x, y)
    if region:
        rx, ry, rw, rh = region
        assert rx <= x and ry <= y and x + width <= rx + rw and y + 8 <= ry + rh, (label, region)
    TEXT_AUDIT.append({'text': label, 'box': [x, y, width, 8], 'region': region})
    d = ImageDraw.Draw(im)
    for ch in label:
        code = ord(ch)
        assert code < 256
        for dx in range(5):
            value = FONT[code * 5 + dx]
            for dy in range(8):
                if value & (1 << dy):
                    d.point((x + dx, y + dy), fill=color)
        x += 6


def panel(im, box, active=False, disabled=False):
    x, y, w, h = box
    assert x >= 0 and y >= 0 and x + w <= im.width and y + h <= im.height
    c = 5
    sw, sh = FRAME.size
    slices = [(0, 0, c, c, x, y, c, c), (sw-c, 0, c, c, x+w-c, y, c, c), (0, sh-c, c, c, x, y+h-c, c, c), (sw-c, sh-c, c, c, x+w-c, y+h-c, c, c), (c, 0, sw-2*c, c, x+c, y, w-2*c, c), (c, sh-c, sw-2*c, c, x+c, y+h-c, w-2*c, c), (0, c, c, sh-2*c, x, y+c, c, h-2*c), (sw-c, c, c, sh-2*c, x+w-c, y+c, c, h-2*c), (c, c, sw-2*c, sh-2*c, x+c, y+c, w-2*c, h-2*c)]
    for sx, sy, sww, shh, dx, dy, dw, dh in slices:
        patch = FRAME.crop((sx, sy, sx+sww, sy+shh)).resize((dw, dh), Image.Resampling.NEAREST)
        im.paste(patch, (dx, dy), patch)
    if disabled:
        crop = im.crop((x, y, x+w, y+h))
        crop = Image.blend(crop, Image.new('RGB', crop.size, INK), .48)
        im.paste(crop, (x, y))
    if active:
        ImageDraw.Draw(im).rectangle((x+2, y+2, x+w-3, y+h-3), outline=TARGET)


def bar(im, x, y, w, value, total, color):
    rect(im, (x, y, w, 3), (30, 30, 25))
    if value and total:
        rect(im, (x, y, max(1, w * value // total), 3), color)


def fit(src, box):
    x, y, w, h = box
    scale = min(w / src.width, h / src.height)
    ww, hh = max(1, int(src.width*scale)), max(1, int(src.height*scale))
    return src.resize((ww, hh), Image.Resampling.NEAREST), (x+(w-ww)//2, y+h-hh)


def sprite(a):
    key = ('hero', a['race'], a['class'], a['shirt'], a['trousers']) if a['kind'] == 1 else ('enemy', a['species'])
    if key not in SPRITES:
        if a['kind'] == 1:
            poses = [art.hero(a, p) for p in range(6)]
            boxes = [im.getbbox() for im in poses]
            box = (min(b[0] for b in boxes), min(b[1] for b in boxes), max(b[2] for b in boxes), max(b[3] for b in boxes))
            SPRITES[key] = poses[0].crop(box)
        else:
            SPRITES[key] = art.enemy(a['species'], 0).crop(PROFILES[a['species']])
    return SPRITES[key]


def actor_name(actor_id):
    return NAMES.get(actor_id, f'#{actor_id}')


def event_of(row):
    event = row.get('event')
    return event if isinstance(event, dict) else row


def action_line(row):
    e = event_of(row)
    if e.get('actor_id', -1) < 0:
        return None
    n, t, action = actor_name(e['actor_id']), actor_name(e.get('target_id', -1)), e.get('action', -1)
    if e.get('from_wave', row['wave']) != row['wave']:
        return f'ONDA {e["from_wave"]} CONCLUIDA > {row["wave"]}'
    if action == 1:
        return f'{n}: DEFENDEU'
    if action == 2:
        return f'{n}: POCAO +{e.get("amount", 0)} HP'
    if action == 3:
        return f'{n}: POCAO +{e.get("amount", 0)} MP'
    if action not in (0, 4):
        return None
    if e.get('amount', 0) == 0:
        return f'{n} > {t}: ESQUIVA'
    return f'{n} > {t}: -{e["amount"]}' + (' CRIT' if e.get('critical') else '') + (' *' if action == 4 else '')


def turn_order(row):
    if row['phase']:
        return 'FIM DO ENCONTRO'
    alive = [a for a in row['actors'] if a['kind'] and a['hp']]
    i = next((i for i, a in enumerate(alive) if a['id'] == row['next_actor_id']), 0)
    alive = alive[i:] + alive[:i]
    return ' > '.join(actor_name(a['id']) if a['kind'] == 1 else {0: 'GOB', 1: 'LOB', 2: 'OSS', 3: 'BOSS', 4: 'JAV'}.get(a['species'], 'INI') for a in alive)


def local_turn(row):
    a = row['actors'][row['local']]
    return not row['phase'] and a['hp'] > 0 and row['next_actor_id'] == a['id']


def controls(row):
    mode = row['ui_mode']
    chosen = 'FOCO*' if row.get('selected_action') == 4 else 'ATAQUE'
    labels = ['ATAQUE', 'HABILIDADE', 'DEFENDER', 'ITENS']
    hints = ['alvo', 'abrir', 'guarda', 'HP / MP']
    enabled = [local_turn(row)] * 4
    active = [False] * 4
    if mode == 'target':
        labels, hints = [chosen, '', '', 'CANCELAR'], ['toque alvo', '', '', 'voltar']
        enabled, active = [False, False, False, True], [True, False, False, False]
    elif mode == 'ability':
        labels, hints = ['FOCO*', '', '', 'CANCELAR'], ['2 MP', '', '', 'voltar']
        enabled = [local_turn(row) and row['actors'][row['local']]['mp'] >= 2, False, False, True]
    elif mode == 'items':
        labels, hints = ['VIDA', 'MANA', '', 'CANCELAR'], ['HP', 'MP', '', 'voltar']
        enabled = [local_turn(row), local_turn(row), False, True]
    elif mode == 'pending':
        enabled = [False] * 4
        hints = ['', '', '', '']
    return labels, hints, enabled, active


def event_label(row):
    mode = row['ui_mode']
    if mode == 'pending':
        return 'AGUARDANDO CONFIRMACAO / ACK'
    if mode == 'target':
        return ('FOCO*' if row.get('selected_action') == 4 else 'ATAQUE') + ': TOQUE NO INIMIGO'
    if mode == 'ability':
        return 'FOCO*: HABILIDADE EXPERIMENTAL'
    if mode == 'items':
        return 'ESCOLHA VIDA OU MANA'
    event = action_line(row)
    if event:
        return event
    if row['phase']:
        return 'ENCONTRO CONCLUIDO EM MEMORIA'
    return 'ESCOLHA UMA ACAO' if local_turn(row) else 'AGUARDE A SUA VEZ'


def validate_row(row):
    assert row['ui_mode'] in ('action', 'ability', 'items', 'target', 'pending')
    assert row['local'] in (0, 1)
    assert len(row['actors']) == 6
    eligible = row.get('eligible_slots', [])
    assert len(eligible) == len(set(eligible))
    for slot in eligible:
        actor = row['actors'][slot]
        assert 3 <= slot <= 5 and actor['slot'] == slot and actor['kind'] in (2, 3) and actor['hp'] > 0, ('invalid or dead eligible enemy', row.get('step'), slot)
    if row['ui_mode'] != 'target':
        assert not eligible, ('eligibility outside target mode', row.get('step'))
    click = row.get('click', row.get('pressed'))
    if isinstance(click, (tuple, list)):
        assert len(click) == 2 and 0 <= click[0] < SIZE[0] and 0 <= click[1] < SIZE[1]


def render(row):
    validate_row(row)
    im = Image.new('RGB', SIZE, INK)
    scale = max(ARENA[2] / BG.width, ARENA[3] / BG.height)
    scene = BG.resize((math.ceil(BG.width*scale), math.ceil(BG.height*scale)), Image.Resampling.NEAREST)
    ox, oy = (scene.width-ARENA[2])//2, (scene.height-ARENA[3])//2
    im.paste(scene.crop((ox, oy, ox+ARENA[2], oy+ARENA[3])), ARENA[:2])
    panel(im, (3, 2, 314, 20))
    area = 'CHEFE' if row['wave'] == 3 else f'ONDA {row["wave"]}/2'
    text(im, 0, 8, f'COOP 2P  {area}  LOCAL J{row["local"]+1}', GOLD, center=160)
    text(im, 4, 25, 'ORDEM', MUTED)
    text(im, 40, 25, turn_order(row), GREEN if row['phase'] else WHITE)
    nxt = 'VITORIA EM RAM' if row['phase'] == 1 else 'DERROTA EM RAM' if row['phase'] == 2 else 'VEZ: ' + actor_name(row['next_actor_id'])
    text(im, 4, 34, nxt, BLUE)
    revision = f'R{row["round"]} / T{row["revision"]}'
    text(im, 314-len(revision)*6, 34, revision, MUTED)
    for a in row['actors']:
        if not a['kind'] or a['hp'] <= 0:
            continue  # No corpse is rendered or marked as eligible.
        player = a['kind'] == 1
        b = HEROES[a['slot']] if player else BOSS if a['kind'] == 3 else ENEMIES[a['slot']-3]
        spr, (x, y) = fit(sprite(a), b)
        assert x >= ARENA[0] and y >= ARENA[1] and x+spr.width <= ARENA[0]+ARENA[2] and y+spr.height <= ARENA[1]+ARENA[3]
        d = ImageDraw.Draw(im)
        d.ellipse((x+3, y+spr.height-4, x+spr.width-3, min(147, y+spr.height+1)), fill=(39, 39, 27))
        im.paste(spr, (x, y), spr)
        if not row['phase'] and a['id'] == row['next_actor_id']:
            d.polygon([(x+spr.width//2-3, y-5), (x+spr.width//2+3, y-5), (x+spr.width//2, y-2)], fill=GREEN)
        if player:
            text(im, b[0]-12, b[1]+b[3]-8, f'J{a["slot"]+1}', BLUE)
            if a['slot'] == row['local']:
                d.line((b[0]-12, b[1]+b[3], b[0]-2, b[1]+b[3]), fill=TARGET, width=2)
        else:
            tag = f'{actor_name(a["id"])} {a["hp"]}' if a['kind'] == 3 else f'{actor_name(a["id"])[:3]} {a["hp"]}'
            tw = len(tag)*6
            tx = max(ARENA[0], min(317-tw, b[0]+b[2]//2-tw//2))
            # The original Goblin and Wolf captions extended past the new hit
            # rectangles. Keep every HP caption/bar above the outline instead.
            hit = BOSS_HIT if a['kind'] == 3 else ENEMY_HITS[a['slot']-3]
            ty = hit[1]+hit[3]-13
            rect(im, (tx-1, ty, tw+2, 10), INK)
            text(im, tx, ty+1, tag, GOLD if a['kind'] == 3 else WHITE)
            bar(im, tx, ty+9, tw, a['hp'], a['maxhp'], RED)
    # Outline exactly the half-open native hit rectangle, without overlapping peers.
    for slot in row.get('eligible_slots', []):
        a = row['actors'][slot]
        x, y, w, h = BOSS_HIT if a['kind'] == 3 else ENEMY_HITS[slot-3]
        d = ImageDraw.Draw(im)
        d.rectangle((x, y, x+w-1, y+h-1), outline=TARGET)
        # Corner ticks stay strictly inside the clickable rectangle.
        for xx, yy, dx, dy in [(x+1, y+1, 1, 1), (x+w-2, y+1, -1, 1), (x+1, y+h-2, 1, -1), (x+w-2, y+h-2, -1, -1)]:
            d.line((xx, yy, xx+dx*5, yy), fill=WHITE)
            d.line((xx, yy, xx, yy+dy*5), fill=WHITE)
    panel(im, EVENT)
    text(im, 0, EVENT[1]+4, event_label(row), GOLD, center=160, region=EVENT)
    for i, b in enumerate(STATS):
        a = row['actors'][i]
        panel(im, b, active=a['id'] == row['next_actor_id'] and not row['phase'])
        x, y, w, h = b
        title = f'J{i+1} {CLASSES[a["class"]]}' + (' LOCAL' if i == row['local'] else '')
        text(im, x+5, y+4, title, BLUE if i == row['local'] else GOLD, region=b)
        text(im, x+5, y+14, f'HP {a["hp"]}/{a["maxhp"]} MP {a["mp"]}/{a["maxmp"]}', WHITE, region=b)
        if a['guard']:
            text(im, x+w-12, y+4, 'D', GOLD, region=b)
    labels, hints, enabled, active = controls(row)
    for i, b in enumerate(BUTTONS):
        panel(im, b, active=active[i], disabled=not enabled[i] and not active[i])
        x, y, w, h = b
        if labels[i]:
            text(im, 0, y+7, labels[i], GOLD if enabled[i] or active[i] else DISABLED, center=x+w//2, region=b)
        if hints[i]:
            text(im, 0, y+17, hints[i], MUTED if enabled[i] or active[i] else DISABLED, center=x+w//2, region=b)
    text(im, 0, 232, 'PROTOTIPO LOCAL | *FOCO EXPERIMENTAL', MUTED, center=160)
    click = row.get('click', row.get('pressed'))
    if isinstance(click, (tuple, list)):
        x, y = click
        d = ImageDraw.Draw(im)
        d.ellipse((x-5, y-5, x+5, y+5), outline=(255, 255, 255), width=1)
        d.line((x-2, y, x+2, y), fill=GOLD)
        d.line((x, y-2, x, y+2), fill=GOLD)
    return im


def validate_geometry():
    records = []
    for name, boxes in [('actions', BUTTONS), ('three_enemies', ENEMY_HITS), ('boss', [BOSS_HIT])]:
        for i, box in enumerate(boxes):
            x, y, w, h = box
            assert x >= 0 and y >= 0 and x+w <= 320 and y+h <= 240
            assert all(not bounds_intersect(box, other) for other in boxes[i+1:])
        counts = [0] * len(boxes)
        for y in range(240):
            for x in range(320):
                hits = [i for i, (bx, by, bw, bh) in enumerate(boxes) if bx <= x < bx+bw and by <= y < by+bh]
                assert len(hits) <= 1
                if hits:
                    counts[hits[0]] += 1
        assert counts == [w*h for x, y, w, h in boxes]
        records.append({'group': name, 'rectangles': boxes, 'classified_pixels': 76800, 'hit_pixels': counts, 'overlap': False})
    for box in BUTTONS:
        assert not bounds_intersect(box, EVENT)
        assert all(not bounds_intersect(box, other) for other in STATS)
    return records


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(trace_path, out):
    rows = [json.loads(line) for line in trace_path.read_text().splitlines() if line.strip()]
    assert rows, 'Trace is empty'
    for row in rows:
        validate_row(row)
    snapshot_before = json.dumps(rows, sort_keys=True)
    native = [render(row) for row in rows]
    assert snapshot_before == json.dumps(rows, sort_keys=True), 'Renderer mutated a trace snapshot'
    geometry = validate_geometry()
    out.mkdir(parents=True, exist_ok=True)
    target_indices = [i for i, r in enumerate(rows) if r['ui_mode'] == 'target' and r['wave'] != 3 and r.get('selected_action') == 0]
    target_idx = next((i for i in target_indices if rows[i].get('click', rows[i].get('pressed')) is not None), target_indices[0])
    boss_idx = next(i for i, r in enumerate(rows) if r['ui_mode'] == 'target' and r['wave'] == 3)
    selected = [('target', target_idx), ('boss', boss_idx)]
    for name, idx in selected:
        native[idx].save(out / f'{name}-native.png')
        native[idx].resize((640, 480), Image.Resampling.NEAREST).save(out / f'{name}-2x.png')
    font_path = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
    title = ImageFont.truetype(str(font_path), 21)
    body = ImageFont.truetype(str(font_path), 14)
    small = ImageFont.truetype(str(font_path), 12)
    sheet = Image.new('RGB', (1340, 650), (17, 28, 30))
    d = ImageDraw.Draw(sheet)
    d.text((20, 12), 'protótipo local · seleção de alvo por toque', font=title, fill=GOLD)
    d.text((20, 43), 'Paisagem 320 × 240 · ampliação exata 2× · personagem local, HP/MP e ordem visíveis', font=body, fill=WHITE)
    for col, (name, idx) in enumerate(selected):
        x = 20 + col*660
        d.text((x, 73), 'Ataque → toque em um inimigo vivo' if name == 'target' else 'Cenário separado: chefe → um alvo elegível', font=body, fill=BLUE)
        sheet.paste(native[idx].resize((640, 480), Image.Resampling.NEAREST), (x, 98))
    d.text((20, 592), 'Contorno verde = alvo elegível. CANCELAR volta ao menu. Círculo branco = toque ilustrativo.', font=body, fill=WHITE)
    d.text((20, 614), 'Foco é provisório e experimental. Replay local; sem instalação, rádio ou medição de FPS de hardware.', font=small, fill=MUTED)
    sheet.save(out / 'coop-touch-targeting-2x.png')
    frames = []
    durations = []
    # The boss is a standalone test fixture, not sequential encounter progress.
    # It is shown in the static sheet and all-state golden, outside this GIF.
    gif_indices = [i for i, r in enumerate(rows) if r['wave'] != 3]
    for sequence, idx in enumerate(gif_indices):
        row = rows[idx]
        canvas = Image.new('RGB', (680, 650), (17, 28, 30))
        d = ImageDraw.Draw(canvas)
        d.text((20, 11), 'protótipo local · ações e alvos por toque', font=title, fill=GOLD)
        d.text((20, 43), '320 × 240 em 2× · ritmo ilustrativo, sem tempo de placa medido', font=small, fill=WHITE)
        d.text((20, 65), f'Passo {sequence+1}/{len(gif_indices)} · {row.get("step", row.get("label", row["ui_mode"]))}', font=small, fill=BLUE)
        canvas.paste(native[idx].resize((640, 480), Image.Resampling.NEAREST), (20, 87))
        status = str(row.get('status', '')).strip()
        if sequence and row['revision'] > rows[gif_indices[sequence-1]]['revision']+1:
            status = 'Após os turnos dos inimigos, a vez passa para J1. ' + status
        # Status is auxiliary narration; never silently alter combat snapshots.
        if status:
            words, lines, line = status.split(), [], ''
            for word in words:
                candidate = (line + ' ' + word).strip()
                if d.textlength(candidate, font=small) > 635 and line:
                    lines.append(line)
                    line = word
                else:
                    line = candidate
            if line:
                lines.append(line)
            for n, line in enumerate(lines[:2]):
                d.text((20, 577+n*16), line, font=small, fill=WHITE)
        d.text((20, 614), 'Foco experimental · círculo branco = toque ilustrativo · sem instalação', font=small, fill=MUTED)
        frames.append(canvas)
        # Uniform presentation pacing; timestamps are preserved only as evidence.
        durations.append(1200 if row['ui_mode'] in ('target', 'pending', 'ability') else 1000)
    gif = out / 'coop-touch-flow-local.gif'
    frames[0].save(gif, save_all=True, append_images=frames[1:], duration=durations, loop=0, optimize=True, disposal=2)
    encoded = Image.open(gif)
    actual_duration = 0
    for i in range(encoded.n_frames):
        encoded.seek(i)
        actual_duration += encoded.info['duration']
    assert actual_duration == sum(durations)
    # One contact sheet is a readable golden for every actual serialized UI state.
    cols = 3
    contact = Image.new('RGB', (cols*340, math.ceil(len(rows)/cols)*266), INK)
    cd = ImageDraw.Draw(contact)
    for i, (row, im) in enumerate(zip(rows, native)):
        x, y = (i % cols)*340, (i//cols)*266
        cd.text((x+4, y+3), f'{i+1}: {row.get("step", row.get("label", row["ui_mode"]))}', font=small, fill=GOLD)
        contact.paste(im, (x+4, y+23))
    contact.save(out / 'ui-state-goldens.png')
    unique_text = {json.dumps(entry, sort_keys=True): entry for entry in TEXT_AUDIT}
    checks = {'result': 'PASS', 'scope': 'Local trace renderer and native geometry only', 'native_size': SIZE, 'trace_rows': len(rows), 'rendered_native_frames': len(native), 'all_text_in_bounds': True, 'text_goldens_checked': len(unique_text), 'no_dead_eligible_enemies': True, 'trace_snapshots_unchanged': True, 'geometry': geometry, 'static_indices': dict(selected), 'gif_trace_indices': gif_indices, 'gif_encoded_frames': encoded.n_frames, 'gif_duration_ms': actual_duration, 'pacing_is_illustrative': True, 'hardware_verified': False}
    (out / 'render-checks.json').write_text(json.dumps(checks, indent=2) + '\n')
    (out / 'text-bounds-goldens.json').write_text(json.dumps(list(unique_text.values()), indent=2) + '\n')
    sources = [Path(__file__), Path(__file__).with_name('test_render_touch_flow.py'), Path(__file__).with_name('CoopTouchUi.h'), Path(__file__).with_name('RENDERING.md'), HELPER, trace_path, SRC/'cartao/RPGPOKET/artes.pak', FW/'AssetCatalog.h', FW/'EnemyScale.h', FW/'RaceArt.h', FONT_SOURCE, FRAME_PATH, font_path]
    manifest = {'purpose': 'Read-only local cooperative touch-flow rendering', 'installed': False, 'production_changed': False, 'source_files': [{'path': str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p), 'sha256': sha(p), 'bytes': p.stat().st_size} for p in sources], 'art': {'source': 'Existing verified PKA1 RGB565/indexed artwork, race masks, and original nine-slice UI frame', 'asset_ids': sorted(art.USED), 'pack_crc32': hex(art.crc)}, 'fonts': {'native': 'Original firmware 5x8 bitmap glyphs; 6px advance, no substituted native font', 'presentation': str(font_path)}, 'geometry': checks['geometry'], 'trace_rows': len(rows), 'state_hashes': [r['state_hash'] for r in rows], 'static_rows': {name: {'index': idx, 'step': rows[idx].get('step'), 'state_hash': rows[idx]['state_hash']} for name, idx in selected}, 'gif': {'frames': encoded.n_frames, 'trace_indices': gif_indices, 'duration_ms': actual_duration, 'pacing': 'Illustrative: 1000–1200 ms per state; not observed latency or FPS', 'boss_policy': 'Standalone boss fixture excluded from sequential GIF; static sheet labels it a separate scenario', 'touch_marker': 'Illustrative cursor drawn only from trace click/pressed coordinates'}, 'limitations': ['Local prototype, not installed firmware', 'No real network transport, radio, touch controller, or hardware timing claim', 'Foco is an experimental placeholder, not the final powers system', 'Only live trace-eligible enemies are outlined; no speculative damage or future state is rendered'], 'libraries': {'python': platform.python_version(), 'Pillow': Image.__version__}}
    files = [out/'target-native.png', out/'target-2x.png', out/'boss-native.png', out/'boss-2x.png', out/'coop-touch-targeting-2x.png', gif, out/'ui-state-goldens.png', out/'render-checks.json', out/'text-bounds-goldens.json']
    manifest['outputs'] = [{'path': str(p.relative_to(ROOT)), 'sha256': sha(p), 'bytes': p.stat().st_size} for p in files]
    (out / 'render-manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps(checks, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trace', type=Path, default=ROOT/'output/coop-ui-targeting/ui-trace.jsonl')
    parser.add_argument('--out', type=Path, default=ROOT/'output/coop-ui-targeting')
    args = parser.parse_args()
    run(args.trace, args.out)
