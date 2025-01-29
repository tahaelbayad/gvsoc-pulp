vp_files(FILES "bootrom.bin"
    PREFIX "pulp/snitch"
    )

vp_model(NAME pulp.snitch.sequencer
    SOURCES "sequencer.cpp"
)

vp_model(NAME pulp.snitch.mesh_FIFOs
    SOURCES "mesh_FIFOs.cpp"
)