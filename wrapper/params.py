# The concept of a group is to improve DX

class Params:
    def __init__(self, prefix, group=None):
        self.group = group
        self.prefix = prefix
        self.params = {}

    def update(self, key, value):
        self.params[key] = value

    def serialize(self):
        self.prefix = self.prefix + "." + self.group if self.group != None else self.prefix
        return "".join("%s.%s = %s\n" % (self.prefix, k, v) for _, (k, v) in enumerate(self.params.items()))


def __test__():
    # Single level
    single_level = Params("Cimager")
    single_level.update("gridder", 'WProjection')
    single_level.update("gridder.snapshotimaging", 'false')
    
    single_ser = single_level.serialize()
    expected_single = """Cimager.gridder = WProjection
Cimager.gridder.snapshotimaging = false
"""

    assert single_ser == expected_single, f"\nExpected:\n{expected_single} \nActual:\n{single_ser}" 

    # Nested
    expected_nested = """Cimager.gridder.WProject.wmax = 35000
Cimager.gridder.WProject.nwplanes = 257
Cimager.gridder.WProject.oversample = 7
Cimager.gridder.WProject.maxsupport = 512
Cimager.gridder.WProject.variablesupport = true
"""
    nested = Params("Cimager.gridder", "WProject")
    nested.update("wmax", 35000)
    nested.update("nwplanes", 257)
    nested.update("oversample", 7)
    nested.update("maxsupport", 512)
    nested.update("variablesupport", "true")

    nested_ser = nested.serialize()

    assert nested_ser == expected_nested, f"\nExpected:\n{expected_nested} \nActual:\n{nested_ser}" 
    
    print("All assertions passed: params.py")

# __test__()
