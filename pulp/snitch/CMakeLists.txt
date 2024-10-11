vp_files(FILES "bootrom.bin"
    PREFIX "pulp/snitch"
    )

vp_model(NAME pulp.snitch.sequencer
    SOURCES "sequencer.cpp"
)

vp_model(
    NAME pulp.snitch.light_redmule
    SOURCES "light_redmule.cpp"

)