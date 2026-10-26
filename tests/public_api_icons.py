#!/usr/bin/env python3
import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import fetch_public_api_icons as fetch
import prepare_public_api_icons as art


class IconPackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lock=fetch.load_lock()
        cls.result=art.build()

    def test_rebuild_is_byte_deterministic(self):
        self.assertEqual(self.result.files,art.build().files)
        for name,data in self.result.files.items():
            self.assertEqual((ROOT/name).read_bytes(),data,name)

    def test_build_is_offline(self):
        with patch('urllib.request.build_opener',side_effect=AssertionError('Network prohibited')):
            self.assertEqual(art.build().report,self.result.report)

    def test_geometry_budget(self):
        report=self.result.report
        self.assertEqual((report['icon_count'],report['variants']),(16,32))
        self.assertEqual(report['flash_data_bytes'],12832)
        self.assertEqual(report['maximum_caller_workspace_bytes'],2048)
        # Integration flash delta is measured from the ESP32 binary, not this proposal manifest.
        self.assertEqual(report['art_catalog_entries_changed'],0)
        for row in report['icons']:
            self.assertEqual(row['plane_bytes'],row['width']*row['height']//2)
            self.assertGreater(row['transparent_pixels'],row['width'])
            self.assertLess(row['transparent_pixels'],row['width']*row['height']-row['width'])

    def test_palette_uses_existing_key_only_at_zero(self):
        colors=struct.unpack('<16H',art.PALETTE)
        self.assertEqual(colors[0],0xf81f)
        self.assertNotIn(0xf81f,colors[1:])

    def test_individual_attribution_and_immutable_commits(self):
        self.assertEqual({r['author'] for r in self.lock['icons']},{'Lorc','Delapouite','sbed','Willdabeast','Carl Olsen'})
        credits=self.result.files['assets/public-api-icons1/CREDITS.md'].decode()
        for row in self.lock['icons']:
            self.assertIn(row['author'],credits)
            self.assertIn(row['source_page'],credits)
            self.assertEqual(row['license'],'CC-BY-3.0')
            self.assertIn(self.lock['game_icons']['commit'],row['upstream_url'])

    def test_reject_changed_source(self):
        files=fetch.retained_files(self.lock)
        for name in files:
            changed=dict(files);changed[name]+=b' '
            with self.assertRaises(ValueError,msg=name):fetch.verify_files(self.lock,changed)

    def test_reject_untrusted_svg(self):
        bodies=['<script>alert(1)</script>','<image href="https://example.org/a.png"/>',
                '<path fill="currentColor" d="M0 0" onclick="evil()"/>',
                '<path fill="url(http://example.org/x)" d="M0 0"/>',
                '<!DOCTYPE svg><path fill="currentColor" d="M0 0"/>',
                '<g><path fill="currentColor" d="M0 0"/></g>',
                'unexpected text<path fill="currentColor" d="M0 0"/>','']
        for body in bodies:
            with self.assertRaises((ValueError,fetch.ET.ParseError),msg=body):fetch.validate_body(body)

    def test_reject_changed_snapshot(self):
        with self.assertRaises(ValueError):fetch.subset_bytes(b'{}',self.lock)

    def test_acquisition_contract_rebuilds_only_selected_files(self):
        files=fetch.retained_files(self.lock)
        subset=json.loads(files['sources/iconify-subset.json'])
        snapshot=fetch.json_bytes(dict(subset,info={'license':{'spdx':'CC-BY-3.0'}}))
        lock=copy.deepcopy(self.lock);lock['iconify']['snapshot_sha256']=fetch.sha(snapshot)
        responses={lock['iconify']['snapshot_url']:snapshot,
                   lock['game_icons']['license_url']:files['licenses/CC-BY-3.0-upstream.txt']}
        responses.update({row['upstream_url']:files[f"sources/original/{row['slug']}.svg"] for row in lock['icons']})
        with tempfile.TemporaryDirectory() as directory:
            base=Path(directory)
            with patch.object(fetch,'download',side_effect=lambda url,limit:responses[url]) as download:
                fetch.fetch(lock,base)
            self.assertEqual(download.call_count,18)
            self.assertEqual(files,fetch.retained_files(lock,base))
            self.assertEqual(len(list(p for p in base.rglob('*') if p.is_file())),18)

    def test_failed_acquisition_does_not_write_any_sources(self):
        with tempfile.TemporaryDirectory() as directory:
            base=Path(directory)
            with patch.object(fetch,'download',return_value=b'changed response'):
                with self.assertRaises(ValueError):fetch.fetch(self.lock,base)
            self.assertEqual(list(base.iterdir()),[])

    def test_unapproved_host_and_redirect_rejected(self):
        for url in ('http://raw.githubusercontent.com/file','https://raw.githubusercontent.com.evil/file','https://example.org/file'):
            with self.assertRaises(ValueError):fetch.download(url,100)
        with self.assertRaises(ValueError):fetch.NoRedirects().redirect_request(None,None,302,'',{},'https://example.org/')

    def test_lock_rejects_missing_credits_other_license_and_paths(self):
        for field,value in [('slug','../../evil'),('author',''),('license','MIT'),('upstream_url','https://example.org/icon.svg')]:
            lock=copy.deepcopy(self.lock);lock['icons'][0][field]=value
            with tempfile.TemporaryDirectory() as directory:
                path=Path(directory);(path/'source-lock.json').write_bytes(fetch.json_bytes(lock))
                with self.assertRaises(ValueError):fetch.load_lock(path)

    def test_size_and_empty_geometry_rejected(self):
        for size in (0,1,16,25,33,2**32):
            with self.assertRaises(ValueError):art.coverage(bytes(16384),size)
        with self.assertRaises(ValueError):art.coverage(b'',32)

    def test_provider_is_confined_to_the_view(self):
        self.assertIn('PublicApiIcons.h',(ROOT/'firmware/RPG_POKET_2/View.h').read_text())
        for path in (ROOT/'firmware/RPG_POKET_2').rglob('*'):
            if path.suffix in ('.h','.cpp','.ino') and path.name not in ('PublicApiIconData.h','PublicApiIcons.h'):
                if path.name!='View.h':
                    self.assertNotIn('PublicApiIcons.h',path.read_text(errors='replace'),str(path))
                self.assertNotIn('PublicApiIconData.h',path.read_text(errors='replace'),str(path))


if __name__=='__main__':unittest.main()
