#
# Copyright (C) 2024 ETH Zurich and University of Bologna
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Author: Chi Zhang <chizhang@iis.ee.ethz.ch>

import gvsoc.systree

class LightRedmule(gvsoc.systree.Component):

    def __init__(self,
                parent: gvsoc.systree.Component,
                name: str,
                redmule_id: int,
                tcdm_bank_width: int,
                tcdm_bank_number: int,
                elem_size: int,
                ce_height: int,
                ce_width: int,
                ce_pipe: int,
                queue_depth: int=8,
                fold_tiles_mapping: int=0,
                compute_format: str="None"):

        super().__init__(parent, name)

        self.set_component('pulp.snitch.light_redmule')

        compute_type = 0
        if compute_format == "None":
            compute_type = 0
        elif compute_format == "Integer":
            compute_type = 1
            pass

        self.add_properties({
            'redmule_id'        : redmule_id,
            'tcdm_bank_width'   : tcdm_bank_width,
            'tcdm_bank_number'  : tcdm_bank_number,
            'elem_size'         : elem_size,
            'ce_height'         : ce_height,
            'ce_width'          : ce_width,
            'ce_pipe'           : ce_pipe,
            'queue_depth'       : queue_depth,
            'fold_tiles_mapping': fold_tiles_mapping,
            'compute_type'      : compute_type,
        })

    def i_INPUT(self) -> gvsoc.systree.SlaveItf:
        return gvsoc.systree.SlaveItf(self, 'input', signature='io')

    def o_TCDM(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('tcdm', itf, signature='io')