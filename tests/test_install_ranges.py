#!/usr/bin/env python3
import sys,pathlib,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from package_install import check_ranges
class Ranges(unittest.TestCase):
    def test_protected_gap(self):check_ranges([(0,0x250000),(0x35a000,0x400000)])
    def test_cross_identity(self):
        with self.assertRaises(ValueError):check_ranges([(0,0x700000)])
    def test_erase_alignment(self):
        with self.assertRaises(ValueError):check_ranges([(0x355000,4097)])
    def test_bounds(self):
        with self.assertRaises(ValueError):check_ranges([(0x35a000,0x4a6001)])
    def test_overlap(self):
        with self.assertRaises(ValueError):check_ranges([(0,0x1001),(0x1000,4096)])
if __name__=='__main__':unittest.main()
