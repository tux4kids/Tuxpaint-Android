LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_croco

LOCAL_SRC_FILES :=  			\
		src/cr-utils.c 		\
		src/cr-input.c 		\
		src/cr-enc-handler.c 	\
		src/cr-num.c 		\
		src/cr-rgb.c 		\
		src/cr-token.c 		\
		src/cr-tknzr.c 		\
		src/cr-term.c 		\
		src/cr-attr-sel.c 	\
		src/cr-pseudo.c 	\
		src/cr-additional-sel.c \
		src/cr-simple-sel.c 	\
		src/cr-selector.c 	\
		src/cr-doc-handler.c 	\
		src/cr-parser.c 	\
		src/cr-declaration.c 	\
		src/cr-statement.c 	\
		src/cr-stylesheet.c 	\
		src/cr-cascade.c 	\
		src/cr-om-parser.c	\
		src/cr-style.c 		\
		src/cr-sel-eng.c 	\
		src/cr-fonts.c 		\
		src/cr-prop-list.c 	\
		src/cr-parsing-location.c \
		src/cr-string.c 	\
		$(NULL)

LOCAL_C_INCLUDES := 				\
	$(LOCAL_PATH)/src		        \
	$(LOCAL_PATH)		           	\
	$(NULL)

LOCAL_EXPORT_C_INCLUDES := 			\
	$(LOCAL_PATH)/include           	\
	$(NULL)

LOCAL_CFLAGS := 				\
	-DHAVE_CONFIG_H				\
	$(NULL)

LOCAL_SHARED_LIBRARIES := 	\
	tuxpaint_glib		\
	tuxpaint_xml2		\
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
