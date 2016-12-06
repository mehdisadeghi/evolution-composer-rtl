list(APPEND SOURCES
	module-composer-rtl.c
	m-msg-composer-extension.h
	m-msg-composer-extension.c
)

add_library(module-composer-rtl MODULE ${SOURCES})

set_target_properties(module-composer-rtl PROPERTIES PREFIX "")

target_compile_definitions(module-composer-rtl PRIVATE
	G_LOG_DOMAIN=\"module-composer-rtl\"
)

target_compile_options(module-composer-rtl PUBLIC
	${EVOLUTION_MAIL_CFLAGS}
)

target_include_directories(module-composer-rtl PUBLIC
	${EVOLUTION_MAIL_INCLUDE_DIRS}
)

target_link_libraries(module-composer-rtl
	${EVOLUTION_MAIL_LDFLAGS}
)

install(TARGETS module-composer-rtl
	DESTINATION ${EVOLUTION_MODULE_DIR}
)

include_directories(/home/mehdi/src/gnome/evolution/src/modules/webkit-editor/)