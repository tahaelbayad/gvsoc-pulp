import gvsoc.systree

class FIFO_inout(gvsoc.systree.Component):

    def __init__(self, parent: gvsoc.systree.Component, name: str):

        super().__init__(parent, name)

        self.set_component('pulp.snitch.FIFO_inout')


    def i_INPUT(self) -> gvsoc.systree.SlaveItf:
        return gvsoc.systree.SlaveItf(self, 'i_dma_req', signature='wire<fifo_reqrsp_t *>')
    
    def o_FIFO_out(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('o_fifo_out', itf, signature='wire<fifo_reqrsp_t *>')

    def o_FIFO_in(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('o_fifo_in', itf, signature='wire<fifo_reqrsp_t *>')

