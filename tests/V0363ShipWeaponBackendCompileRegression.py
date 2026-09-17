from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
backend = (root / 'src/core/WeaponAudioPipelineBackend.cpp').read_text(encoding='utf-8')
header = (root / 'include/StarfieldDualSense/ShipWeaponSemanticCatalog.h').read_text(encoding='utf-8')

checks = {
    'catalog exposes read-only size accessor': 'std::size_t size() const noexcept' in header,
    'backend uses public catalog size accessor': 'batch.shipWeaponCatalog.size()' in backend,
    'backend does not access private catalog storage': 'batch.shipWeaponCatalog.events' not in backend,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    for name in failed:
        print(f'FAIL {name}')
    sys.exit(1)
for name in checks:
    print(f'PASS {name}')
print('PASS v0.3.63 ship weapon backend compile contract regression')
