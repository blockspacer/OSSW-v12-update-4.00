--- u-boot-2013.07/config.mk	2013-12-13 13:49:25.356958498 -0500
+++ uboot.new/config.mk	2013-12-05 12:17:54.979503948 -0500
@@ -201,8 +201,9 @@
 
 gccincdir := $(shell $(CC) -print-file-name=include)
 
-CPPFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS)		\
-	-D__KERNEL__
+CPPFLAGS = $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS)		\
+	-D__KERNEL__ 					\
+	-D__UBOOT__                                     
 
 # Enable garbage collection of un-used sections for SPL
 ifeq ($(CONFIG_SPL_BUILD),y)
@@ -239,6 +240,7 @@
 CPPFLAGS += -DCONFIG_SPL_BUILD
 endif
 
+CPPFLAGS := $(CPPFLAGS) -include ${SPXINC}/projdef.h -Wall -Werror -Wextra
 # Does this architecture support generic board init?
 ifeq ($(__HAVE_ARCH_GENERIC_BOARD),)
 ifneq ($(CONFIG_SYS_GENERIC_BOARD),)
@@ -260,6 +262,14 @@
 	-isystem $(gccincdir) -pipe $(PLATFORM_CPPFLAGS)
 
 CFLAGS := $(CPPFLAGS) -Wall -Wstrict-prototypes
+CFLAGS += -Wextra -Werror
+
+# Fix the following to Harden the Bootloader
+CFLAGS += -Wno-unused-parameter -Wno-old-style-declaration -Wno-empty-body -Wno-type-limits
+CFLAGS += -Wno-sign-compare -Wno-missing-field-initializers
+#if __GNUC__ >= 9
+CFLAGS += -Wno-address-of-packed-member  -Wno-cast-function-type -Wno-implicit-fallthrough
+# endif
 
 ifdef BUILD_TAG
 CFLAGS += -DBUILD_TAG='"$(BUILD_TAG)"'
@@ -361,12 +371,14 @@
 $(obj)%.s:	%.S
 	$(CPP) $(ALL_AFLAGS) -o $@ $<
 $(obj)%.o:	%.S
-	$(CC)  $(ALL_AFLAGS) -o $@ $< -c
+	@echo "CC  `pwd`/$< "   
+	@$(CC)  $(ALL_AFLAGS) -o $@ $< -c
 $(obj)%.o:	%.c
 ifneq ($(CHECKSRC),0)
 	$(CHECK) $(CHECKFLAGS) $(ALL_CFLAGS) $<
 endif
-	$(CC)  $(ALL_CFLAGS) -o $@ $< -c
+	@echo "CC  `pwd`/$< "   
+	@$(CC)  $(ALL_CFLAGS) -o $@ $< -c
 $(obj)%.i:	%.c
 	$(CPP) $(ALL_CFLAGS) -o $@ $< -c
 $(obj)%.s:	%.c
