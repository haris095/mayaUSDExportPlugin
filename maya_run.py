from maya import cmds, standalone
standalone.initialize()

cmds.loadPlugin("D:/TestProjects/MayaProjects/mayaUSDMan/build/Debug/usdplugin.mll")

cmds.file("D:/Groom_usd_test/groom_length_test.mb", open=True, force=True)

cmds.pickExample()
standalone.uninitialize() 

