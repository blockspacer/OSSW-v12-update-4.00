diff -Naur linux_org/drivers/mtd/spichips/spimtd.c linux_new/drivers/mtd/spichips/spimtd.c
--- linux_org/drivers/mtd/spichips/spimtd.c	1969-12-31 19:00:00.000000000 -0500
+++ linux_new/drivers/mtd/spichips/spimtd.c	2016-10-03 17:35:02.030175345 -0400
@@ -0,0 +1,246 @@
+/*
+ * Copyright (C) 2007 American Megatrends Inc
+ *
+ * This program is free software; you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation; either version 2 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
+ */
+
+#include "spiflash.h"
+
+static struct mtd_info * spi_flash_probe(struct map_info *map);
+static void spi_flash_destroy(struct mtd_info *mtd);
+
+static struct mtd_chip_driver spi_flash_chipdrv = 
+{
+	.probe = spi_flash_probe,
+	.destroy = spi_flash_destroy,
+	.name = "spi_probe",
+	.module = THIS_MODULE
+};
+
+unsigned long ractrends_spiflash_flash_id[MAX_SPI_BANKS];
+unsigned long ractrends_spiflash_flash_size[MAX_SPI_BANKS];
+unsigned char ractrends_spiflash_address_mode[MAX_SPI_BANKS];
+
+
+/*------------------------------------------------------------------------------*/
+/*                               Probe Function       			                */
+/*------------------------------------------------------------------------------*/
+static
+int
+probe_spi_chips(struct map_info *map,struct spi_flash_private *private,
+							struct spi_flash_info *spi_info)
+{
+	int bank = map->map_priv_1;
+	int req = 0;
+	int ctrl,chip;
+	int gotindex;
+	struct spi_chip_driver *chip_drv;
+	struct spi_ctrl_driver *ctrl_drv;
+	
+	gotindex=0;
+
+	/* For Every Controller driver */
+	ctrl = 0;
+	ctrl_drv = get_spi_ctrl_driver_by_index(ctrl);
+	while (ctrl_drv != NULL)
+	{
+		/* For Every Chip driver */
+		chip = 0;
+		chip_drv = get_spi_chip_driver_by_index(chip);
+		while (chip_drv != NULL)
+		{	
+			/* Probe for the chip on the controller */
+			if (chip_drv->probe(bank,ctrl_drv,spi_info) == 0)
+			{
+				/* If success, check if it is the requested index */
+				if (req == gotindex)
+				{
+					private->chip_drv = chip_drv;
+					private->ctrl_drv = ctrl_drv;
+					return 0;
+				}
+				gotindex++;
+			}
+			/* Try Next chip */
+			chip++;
+			chip_drv = get_spi_chip_driver_by_index(chip);
+		}
+		/* Try next controller */
+		ctrl++;
+		ctrl_drv = get_spi_ctrl_driver_by_index(ctrl);
+	}
+	return -1;
+}
+	
+static int configure_spi_clock(struct spi_ctrl_driver *ctrl_drv, int bank, unsigned int clock)
+{
+	if (ctrl_drv->spi_configure_clock == NULL)
+		return 0;
+	
+	return ctrl_drv->spi_configure_clock(bank, clock);
+}
+
+/*--------------------------------------------------------------------------------------*/
+/*                               Interfaces to MTD Module                               */
+/*--------------------------------------------------------------------------------------*/
+static struct mtd_info *
+spi_flash_probe(struct map_info *map)
+{
+	struct mtd_info *mtd;
+	struct spi_flash_private *private;
+
+	struct spi_flash_info spi_info;
+	struct flchip chips;
+	int j,i;
+
+
+	private = kmalloc(sizeof(*private) + sizeof(struct flchip) , GFP_KERNEL);
+	if (!private) 
+	{
+		printk(KERN_WARNING
+		       "%s: kmalloc failed for private structure\n", map->name);
+		return NULL;
+	}
+	memset(private,0,sizeof(*private) + (sizeof(struct flchip)));
+
+	mtd = (struct mtd_info*)kmalloc(sizeof(*mtd), GFP_KERNEL);
+	if (!mtd) 
+	{
+		printk(KERN_WARNING
+		       "%s: kmalloc failed for info structure\n", map->name);
+		return NULL;
+	}
+	memset(mtd, 0, sizeof(*mtd));
+
+	mtd->priv = map;
+
+	memset(&spi_info, 0, sizeof(spi_info));
+	if (probe_spi_chips(map,private,&spi_info) == -1)
+	{
+		printk(KERN_WARNING
+		       "%s: No spi compatible flash device found\n",
+		       map->name);
+		map->fldrv_priv = NULL;
+		kfree(mtd);
+		kfree(private);
+		return NULL;
+	}
+
+	/* Fill flash ID and size in public array */
+	ractrends_spiflash_flash_id[map->map_priv_1]     = ((spi_info.mfr_id << 16) | spi_info.dev_id);
+	ractrends_spiflash_flash_size[map->map_priv_1]   = spi_info.size; 
+	ractrends_spiflash_address_mode[map->map_priv_1] = spi_info.address32;
+
+	configure_spi_clock(private->ctrl_drv, map->map_priv_1, spi_info.max_clock);
+
+	chips.start = 0;
+	chips.state = FL_READY;
+
+
+	/* Fill in the mtd  structures */
+	mtd->size = spi_info.size;
+	mtd->numeraseregions = spi_info.numeraseregions;
+
+	/* Allocate memory for erase regions */
+	mtd->eraseregions = kmalloc(sizeof(struct mtd_erase_region_info) *
+				    mtd->numeraseregions, GFP_KERNEL);
+	if (!mtd->eraseregions) 
+	{
+		printk(KERN_WARNING "%s: Failed to allocate "
+		       "memory for MTD erase region info\n", map->name);
+		kfree(mtd);
+		kfree(private);
+		map->fldrv_priv = NULL;
+		return NULL;
+	}
+
+	/* Fill in the mtd erase structures */
+	for (j = 0; j < spi_info.numeraseregions; j++) 
+	{
+		mtd->eraseregions[j].offset = spi_info.regions[j].offset;
+		mtd->eraseregions[j].erasesize = spi_info.regions[j].erasesize;
+		mtd->eraseregions[j].numblocks = spi_info.regions[j].numblocks;
+		if (mtd->erasesize <  mtd->eraseregions[j].erasesize) 
+			mtd->erasesize = mtd->eraseregions[j].erasesize;
+	}
+
+
+	/* Fill in the remaining mtd structure */
+	mtd->type = MTD_NORFLASH;
+	mtd->flags = MTD_CAP_NORFLASH;
+	mtd->name = map->name;
+	mtd->_erase = spi_flash_erase;
+	mtd->_read = spi_flash_read;
+	mtd->_write = spi_flash_write;
+	mtd->_sync = spi_flash_sync;
+	mtd->_suspend = spi_flash_suspend;
+	mtd->_resume = spi_flash_resume;
+	mtd->_lock = spi_flash_lock;
+	mtd->_unlock = spi_flash_unlock;
+	mtd->writesize = 1;
+
+	/* Fill in the private structure */
+	private->numchips = 1;
+	private->device_type = DEVICE_TYPE_X8;
+	private->interleave = 1;
+	private->address32 = spi_info.address32;
+	private->iomode_read  = private->ctrl_drv->fast_read;
+	private->iomode_write = private->ctrl_drv->fast_write;
+	memcpy(&private->chips, &chips,sizeof(struct flchip) * private->numchips);
+	for (i = 0; i < private->numchips; i++) 
+	{
+		init_waitqueue_head(&private->chips.wq);
+		mutex_init(&private->chips.mutex);
+	}
+
+	/* Fill in the map structure */
+	map->fldrv_priv = private;
+	map->bankwidth = 1; 
+	map->fldrv = &spi_flash_chipdrv;
+	__module_get(THIS_MODULE);
+
+	return mtd;
+}
+
+
+static void 
+spi_flash_destroy(struct mtd_info *mtd)
+{
+	struct map_info *map = mtd->priv;
+	struct spi_flash_private *private = map->fldrv_priv;
+	kfree(private);
+}
+
+
+int __init 
+spi_flash_init(void)
+{
+	register_mtd_chip_driver(&spi_flash_chipdrv);
+	return 0;
+}
+
+void __exit 
+spi_flash_exit(void)
+{
+	unregister_mtd_chip_driver(&spi_flash_chipdrv);
+}
+
+module_init(spi_flash_init);
+module_exit(spi_flash_exit);
+
+MODULE_LICENSE("GPL");
+MODULE_AUTHOR("American Megatrends Inc");
+MODULE_DESCRIPTION("Core SPI with MTD Interface");
+
