import gvsoc.systree

class FIFO_inout(gvsoc.systree.Component):

    def __init__(self, parent: gvsoc.systree.Component, name: str):

        super().__init__(parent, name)

        self.set_component('pulp.snitch.FIFO_inout')


    def i_INPUT(self) -> gvsoc.systree.SlaveItf:
        return gvsoc.systree.SlaveItf(self, 'input', signature='wire<fifo_req_t *>')
    
    def o_FIFO_out(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('fifo_out_resp_o', itf, signature='wire<fifo_resp_t *>')

    def o_FIFO_in(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('fifo_in_resp_o', itf, signature='wire<fifo_resp_t *>')

