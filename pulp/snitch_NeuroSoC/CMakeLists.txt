vp_model(NAME pulp.snitch_NeuroSoC.mesh_FIFOs
    SOURCES "mesh_FIFOs.cpp"
)

set(REDMULE_SRCS
    "src/redmule.cpp"
    )
vp_model(
	NAME pulp.snitch_NeuroSoC.redmule
	SOURCES ${REDMULE_SRCS}
        )