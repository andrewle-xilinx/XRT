// SPDX-License-Identifier: GPL-2.0
/*
 * Xilinx Alveo CU Sub-device Driver
 *
 * Copyright (C) 2020 Xilinx, Inc.
 *
 * Authors: min.ma@xilinx.com
 */

#include "xocl_drv.h"
#include "xrt_cu.h"

#define XCU_INFO(xcu, fmt, arg...) \
	xocl_info(&xcu->pdev->dev, fmt "\n", ##arg)
#define XCU_ERR(xcu, fmt, arg...) \
	xocl_err(&xcu->pdev->dev, fmt "\n", ##arg)
#define XCU_DBG(xcu, fmt, arg...) \
	xocl_dbg(&xcuc->pdev->dev, fmt "\n", ##arg)

struct xocl_cu {
	struct xrt_cu		 base;
	struct platform_device	*pdev;
};

static ssize_t debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	struct xocl_cu *cu = platform_get_drvdata(pdev);
	struct xrt_cu *xcu = &cu->base;
#endif
	/* Place holder for now. */
	return 0;
}

static ssize_t debug_store(struct device *dev,
	struct device_attribute *da, const char *buf, size_t count)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	struct xocl_cu *cu = platform_get_drvdata(pdev);
	struct xrt_cu *xcu = &cu->base;
#endif

	/* Place holder for now. */
	return count;
}

static DEVICE_ATTR_RW(debug);

static ssize_t
cu_stat_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct xocl_cu *cu = platform_get_drvdata(pdev);

	return show_cu_stat(&cu->base, buf);
}
static DEVICE_ATTR_RO(cu_stat);

static struct attribute *cu_attrs[] = {
	&dev_attr_debug.attr,
	&dev_attr_cu_stat.attr,
	NULL,
};

static const struct attribute_group cu_attrgroup = {
	.attrs = cu_attrs,
};

static int cu_probe(struct platform_device *pdev)
{
	xdev_handle_t xdev = xocl_get_xdev(pdev);
	struct xocl_cu *xcu;
	struct resource **res;
	struct xrt_cu_info *info;
	int err = 0;
	void *hdl;
	int i;

	xcu = xocl_drvinst_alloc(&pdev->dev, sizeof(struct xocl_cu));
	if (!xcu)
		return -ENOMEM;

	xcu->pdev = pdev;
	xcu->base.dev = XDEV2DEV(xdev);

	info = XOCL_GET_SUBDEV_PRIV(&pdev->dev);
	memcpy(&xcu->base.info, info, sizeof(struct xrt_cu_info));

	res = vzalloc(sizeof(struct resource *) * xcu->base.info.num_res);
	if (!res) {
		err = -ENOMEM;
		goto err;
	}

	for (i = 0; i < xcu->base.info.num_res; ++i) {
		res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res[i]) {
			err = -EINVAL;
			goto err1;
		}
	}
	xcu->base.res = res;

	err = xocl_kds_add_cu(xdev, &xcu->base);
	if (err) {
		err = 0; //Ignore this error now
		//XCU_ERR(xcu, "Not able to add CU %p to KDS", xcu);
		goto err1;
	}

	switch (info->model) {
	case XCU_HLS:
		err = xrt_cu_hls_init(&xcu->base);
		break;
	case XCU_PLRAM:
		err = xrt_cu_plram_init(&xcu->base);
		break;
	default:
		err = -EINVAL;
	}
	if (err) {
		XCU_ERR(xcu, "Not able to initial CU %p", xcu);
		goto err2;
	}

	if (sysfs_create_group(&pdev->dev.kobj, &cu_attrgroup))
		XCU_ERR(xcu, "Not able to create CU sysfs group");

	platform_set_drvdata(pdev, xcu);

	return 0;

err2:
	(void) xocl_kds_del_cu(xdev, &xcu->base);
err1:
	vfree(res);
err:
	xocl_drvinst_release(xcu, &hdl);
	xocl_drvinst_free(hdl);
	return err;
}

static int cu_remove(struct platform_device *pdev)
{
	xdev_handle_t xdev = xocl_get_xdev(pdev);
	struct xrt_cu_info *info;
	struct xocl_cu *xcu;
	void *hdl;

	xcu = platform_get_drvdata(pdev);
	if (!xcu)
		return -EINVAL;

	(void) sysfs_remove_group(&pdev->dev.kobj, &cu_attrgroup);

	info = &xcu->base.info;
	switch (info->model) {
	case XCU_HLS:
		xrt_cu_hls_fini(&xcu->base);
		break;
	case XCU_PLRAM:
		xrt_cu_plram_fini(&xcu->base);
		break;
	}

	(void) xocl_kds_del_cu(xdev, &xcu->base);

	if (xcu->base.res)
		vfree(xcu->base.res);

	xocl_drvinst_release(xcu, &hdl);

	platform_set_drvdata(pdev, NULL);
	xocl_drvinst_free(hdl);

	return 0;
}

static struct platform_device_id cu_id_table[] = {
	{ XOCL_DEVNAME(XOCL_CU), 0 },
	{ },
};

static struct platform_driver cu_driver = {
	.probe		= cu_probe,
	.remove		= cu_remove,
	.driver		= {
		.name = XOCL_DEVNAME(XOCL_CU),
	},
	.id_table	= cu_id_table,
};

int __init xocl_init_cu(void)
{
	return platform_driver_register(&cu_driver);
}

void xocl_fini_cu(void)
{
	platform_driver_unregister(&cu_driver);
}
