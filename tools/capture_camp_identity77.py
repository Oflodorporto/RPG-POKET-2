"""Compatibility entrypoint for the current personalized camp preview workflow."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('capture_camp_meal7.py')),run_name='__main__')
