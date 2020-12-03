--- u-boot-2013.07/configs/ast2500evb.h	2019-07-30 19:48:36.757003265 +0800
+++ new-u-boot-2013.07/configs/ast2500evb.h	2019-07-30 20:02:04.977003265 +0800
@@ -35,8 +35,9 @@
 	
 /* ECC Support */
 #ifdef CONFIG_SPX_FEATURE_GLOBAL_MEMORY_ECC_ENABLE
-#define CONFIG_SYS_SDRAM_ECC_USED	(CONFIG_SYS_SDRAM_LEN >> 3)
-#define CONFIG_SYS_SDRAM_SYS_USED	(CONFIG_SYS_SDRAM_LEN - CONFIG_SYS_SDRAM_ECC_USED)
+#define CONFIG_SYS_SDRAM_SYS_USED      ((((CONFIG_SYS_SDRAM_LEN >> 20 ) / 9) << 3) << 20)
+#define CONFIG_SYS_SDRAM_ECC_USED      (CONFIG_SYS_SDRAM_SYS_USED  >> 3)
+#define CONFIG_SYS_ECC_MAX_ADDR        ((CONFIG_SYS_SDRAM_SYS_USED - 0x1)&0x3FF00000)
 #define CONFIG_DRAM_ECC
 #endif