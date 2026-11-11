# Throwaway trust files, for tests only

Made with `gfrelease root sign` and `index-keys`, from the keys in `keys/`,
which have no password and are worth nothing. Nothing real trusts them.

- `root/1.root.json`: root keys a, b, c (2 needed), index list key g1.
- `root/2.root.json`: c replaced by d; signed by a, b and d.
- `root/3.root.json`: g1 replaced by g2; signed by a and d.
- `index-keys/1.json`: index key 1, signed by g1.
- `index-keys/2.json`: index key 2, signed by g2.

scripts/tests/test_trust.py and the update checker's tests
(modules/src/m_ver_check/test) read them, so the Python and the C++ side
are checked against the same files.
