from maya import cmds, standalone
standalone.initialize()

cmds.loadPlugin("D:/TestProjects/MayaProjects/mayaUSDMan/build/Debug/usdplugin.mll")

cmds.file("D:/Groom_usd_test/spline_export_scene.mb", open=True, force=True)

cmds.pickExample()
standalone.uninitialize() 

