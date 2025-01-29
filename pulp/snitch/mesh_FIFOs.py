import gvsoc.systree

class mesh_FIFOs(gvsoc.systree.Component):

    def __init__(self, parent: gvsoc.systree.Component, name: str):

        super().__init__(parent, name)

        self.set_component('pulp.snitch.mesh_FIFOs')


    def i_DMA(self) -> gvsoc.systree.SlaveItf:
        return gvsoc.systree.SlaveItf(self, 'i_dma_req', signature='wire<fifo_reqrsp_t *>')
    
    def o_FIFOout(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('o_fifo_out_resp', itf, signature='wire<fifo_reqrsp_t *>')

    def o_FIFOin(self, itf: gvsoc.systree.SlaveItf):
        self.itf_bind('o_fifo_in_resp', itf, signature='wire<fifo_reqrsp_t *>')