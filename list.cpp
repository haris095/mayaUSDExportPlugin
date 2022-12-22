#include <vector>
#include <regex>

#include <maya/MSimple.h>
#include <maya/MLibrary.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MPlug.h>
#include <maya/MPxData.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MSelectionList.h>
#include <maya/MBoundingBox.h>
#include <maya/MIOStream.h>
#include <maya/MTypes.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFileIO.h>
#include <maya/MIOStream.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MEulerRotation.h>
#include <xgen/src/xgsculptcore/api/XgSplineAPI.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

struct{
    VtArray<int> faceVertexCounts;
    VtArray<int> faceVertexIndices;
    VtArray<int> stIndices;
    VtArray<GfVec2f> texCoords;
    VtArray<GfVec3f> extent;
} meshIndexData;

struct{
    VtArray<int> splineVertexCounts;
    VtArray<float> splineWidths;
    VtArray<GfVec3f> splinePoints;
    VtArray<GfVec2f> splineTexCoords;
} splineData;

void getTexCoords(MFnMesh &mesh){
    int numUVs = mesh.numUVs();
    for(int i=0; i<numUVs; i++){
        GfVec2f uv;
        mesh.getUV(i, uv[0], uv[1]);
        meshIndexData.texCoords.push_back(uv);
    }
}

void getFaceVertexIndicesUVData(MObject& obj, MFnMesh &mesh){
    MItMeshPolygon polygonItr(obj);
    MStringArray sets;
    mesh.getUVSetNames(sets);
    bool bUVs = ((sets.length() != 0) && (mesh.numUVs() > 0));
    
    while(!polygonItr.isDone()){
        int vc = polygonItr.polygonVertexCount();
        meshIndexData.faceVertexCounts.push_back(vc);

        for(int i=0; i<vc; i++){
            meshIndexData.faceVertexIndices.push_back(polygonItr.vertexIndex(i));

            if(bUVs){
                int uv_index;
                for(int j=0; j<sets.length(); j++){
                    polygonItr.getUVIndex(i, uv_index, &sets[j]);
                    meshIndexData.stIndices.push_back(uv_index);
                }
            }
        }

        polygonItr.next();
    }

}



DeclareSimpleCommand( pickExample, "Autodesk", "1.0" );

MStatus pickExample::doIt( const MArgList& )
{

    MDagPath node;
    MObject component;
    MSelectionList list;

    std::string nodeName;

    VtArray<GfVec3f> pointsArr;

    //VtArray<int> vertexCountsArr = {3, 3};

    //VtArray<GfVec2f> stPrimArr = {{0.5, 0.5}, {0.6, 0.6}};

    //VtArray<float> widthArr = {0.1, 0.2, 0.3, 0.11, 0.21, 0.31};


    
    VtArray<GfVec3f> NormalsArr;

    std::vector<MFnDagNode> groupDagNodes;
    
    MItDag itDag(MItDag::kDepthFirst);
    MItDag splineDag(MItDag::kDepthFirst, MFn::kPluginShape);

    UsdStageRefPtr stage = UsdStage::CreateNew("D:/Groom_usd_test/Files_usd/mesh.usda");
    MGlobal::displayInfo(MString("Created stage!"));
    UsdGeomXform root_prim = UsdGeomXform::Define(stage, SdfPath("/World"));
    MGlobal::displayInfo(MString("Created Xform!"));
    UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
    while(!splineDag.isDone())
    {
        MFnDagNode splinenodeFn(splineDag.item());
        MObject splinenodeObj = splineDag.item();
        if(splinenodeObj.hasFn(MFn::kPluginShape)){
            std::string name = splinenodeFn.name().asChar();
            if(std::regex_match(name, std::regex("(.*)(_splineDescription)(.*)"))){
                MSelectionList selList = MSelectionList();
                MGlobal::getSelectionListByName(splinenodeFn.name(), selList);
                MObject node_s = MObject();
                selList.getDependNode(0, node_s);

                MFnDependencyNode dep_node = MFnDependencyNode(node_s);
                SdfPath splineNodePath("/World/" + name);
                UsdGeomBasisCurves splineCurves = UsdGeomBasisCurves::Define(stage, splineNodePath);              
                XGenSplineAPI::XgFnSpline _splines;

                MGlobal::displayInfo(MString(" Has spline plugin shape "));
                std::string data;
                MPlug outPlug = splinenodeFn.findPlug("outRenderData");
                MObject outObj = outPlug.asMObject();
                MPxData* objData = MFnPluginData(outObj).data();
                cout << objData->name() << endl;
                if(objData){
                    std::stringstream opaqueStrm;
                    objData->writeBinary(opaqueStrm);
                    data = opaqueStrm.str();

                    cout << data.size() << endl;
                    
                    const unsigned int tail = data.size() % sizeof(unsigned int);
                    const unsigned int padding = (tail > 0) ? sizeof(unsigned int) - tail : 0;
                    const unsigned int nelements = (unsigned int)(data.size() / sizeof(unsigned int) + (tail > 0 ? 1 : 0));

                    size_t sampleSize = nelements * sizeof(unsigned int) - padding;
                    float sampleTime = 0.0f; // this seems to be an irrelevant parameter
                    _splines.load(opaqueStrm, sampleSize ,sampleTime);
                    _splines.executeScript();

                    int curveVertexCount = 0;

                    XGenSplineAPI::XgItSpline splineIt = _splines.iterator();

                    
                    while(!splineIt.isDone()){
                        const unsigned int* primitiveInfos = splineIt.primitiveInfos();
                        const unsigned int  stride = splineIt.primitiveInfoStride();
                        for (unsigned int currCurveIdx = 0; currCurveIdx < splineIt.primitiveCount(); currCurveIdx++){
                            unsigned int length = primitiveInfos[currCurveIdx * stride + 1];
                            unsigned int offset = primitiveInfos[currCurveIdx * stride];
                            int currIdx = offset;
                            splineData.splineVertexCounts.push_back(length);
                            for(unsigned int j=0; j<length;j++){
                                int idx = offset + j;
                                float pos_x = splineIt.positions(0)[idx][0];
                                float pos_y = splineIt.positions(0)[idx][1];
                                float pos_z = splineIt.positions(0)[idx][2];

                                
                                splineData.splinePoints.push_back({pos_x, pos_y, pos_z});
                                
                                splineData.splineWidths.push_back(*splineIt.width());
                            }
                            float u = splineIt.patchUVs()[offset][0];
                            float v = splineIt.patchUVs()[offset][1];
                            splineData.splineTexCoords.push_back({u,v});
                        }
                        splineIt.next();
                    }
                    UsdAttribute splineVertexCountsAttr = splineCurves.CreateCurveVertexCountsAttr();
                    splineVertexCountsAttr.Set<VtArray<int>>(splineData.splineVertexCounts, timeCode);

                    UsdAttribute splinePointsAttr = splineCurves.CreatePointsAttr();
                    splinePointsAttr.Set<VtArray<GfVec3f>>(splineData.splinePoints, timeCode);

                    UsdAttribute basisAttr = splineCurves.CreateBasisAttr();
                    basisAttr.Set(TfToken("bspline"));

                    UsdAttribute doubleSided = splineCurves.CreateDoubleSidedAttr();
                    doubleSided.Set(true);

                    SdfValueTypeName uvValueType = SdfValueTypeNames->TexCoord2fArray;
                    UsdGeomPrimvar primvar = UsdGeomPrimvarsAPI(splineCurves).CreatePrimvar(TfToken("st1"),uvValueType);
                    primvar.Set(splineData.splineTexCoords, timeCode);

                    SdfValueTypeName intType = SdfValueTypeNames->Int;

                    UsdGeomPrimvar endCapsPrimvar = UsdGeomPrimvarsAPI(splineCurves).CreatePrimvar(TfToken("endcaps"),intType);
                    endCapsPrimvar.Set(0, timeCode);

                    SdfValueTypeName colorType = SdfValueTypeNames->Color3f;
                    UsdGeomPrimvar colorPrimvar = UsdGeomPrimvarsAPI(splineCurves).CreatePrimvar(TfToken("displayColor"),colorType);

                    GfVec3f color(1,1,1);
                    colorPrimvar.Set(color, timeCode);

                    UsdAttribute wrapAttr = splineCurves.CreateWrapAttr();
                    wrapAttr.Set(TfToken("pinned"));

                    UsdAttribute widthsAttr = splineCurves.CreateWidthsAttr();
                    widthsAttr.Set<VtArray<float>>(splineData.splineWidths, timeCode);

                }
            }
            
        }
        splineDag.next();
    }
    /*
    while(!splineDag.isDone())
    {
        MFnDagNode splinenodeFn(splineDag.item());
        MObject splinenodeObj = splineDag.item();
        if(splinenodeObj.hasFn(MFn::kPluginShape)){
            std::string name = splinenodeFn.name().asChar();
            if(std::regex_match(name, std::regex("(.*)(_splineDescription)(.*)"))){
                MSelectionList selList = MSelectionList();
                MGlobal::getSelectionListByName(splinenodeFn.name(), selList);
                MObject node_s = MObject();
                selList.getDependNode(0, node_s);

                MFnDependencyNode dep_node = MFnDependencyNode(node_s);
                SdfPath splineNodePath("/World/" + name);
                UsdGeomBasisCurves splineCurves = UsdGeomBasisCurves::Define(stage, splineNodePath);              
                XGenSplineAPI::XgFnSpline _splines;

                MGlobal::displayInfo(MString(" Has spline plugin shape "));
                std::string data;
                MPlug outPlug = splinenodeFn.findPlug("outRenderData");
                MObject outObj = outPlug.asMObject();
                MPxData* objData = MFnPluginData(outObj).data();
                cout << objData->name() << endl;
                if(objData){
                    std::stringstream opaqueStrm;
                    objData->writeBinary(opaqueStrm);
                    data = opaqueStrm.str();

                    cout << data.size() << endl;
                    
                    const unsigned int tail = data.size() % sizeof(unsigned int);
                    const unsigned int padding = (tail > 0) ? sizeof(unsigned int) - tail : 0;
                    const unsigned int nelements = (unsigned int)(data.size() / sizeof(unsigned int) + (tail > 0 ? 1 : 0));

                    size_t sampleSize = nelements * sizeof(unsigned int) - padding;
                    float sampleTime = 0.0f; // this seems to be an irrelevant parameter
                    _splines.load(opaqueStrm, sampleSize ,sampleTime);
                    _splines.executeScript();

                    int curveVertexCount = 0;

                    XGenSplineAPI::XgItSpline splineIt = _splines.iterator();

                    
                    while(!splineIt.isDone()){
                        const uint32_t stride = splineIt.primitiveInfoStride();
                        cout << "stride = " << stride << endl;

                        const uint32_t primitiveCount = splineIt.primitiveCount();
                        const uint32_t* primitiveInfos = splineIt.primitiveInfos();
                        MGlobal::displayInfo(MString("Inside spline loop"));
                        splineData.splineVertexCounts.push_back(splineIt.vertexCount());
                        for (uint32_t i = 0; i < primitiveCount; i++) {
                            const uint32_t offset = primitiveInfos[i * stride];

                            //cout << splineIt.positions()[offset][0] << endl;
                        
                            splineData.splinePoints.push_back({static_cast<float>(splineIt.positions()[offset][0]),
                                static_cast<float>(splineIt.positions()[offset][1]), static_cast<float>(splineIt.positions()[offset][2])});
                        
                            splineData.splineTexCoords.push_back({static_cast<float>(splineIt.texcoords()[offset][0]),
                                splineIt.texcoords()[offset][1]});
                            
                            splineData.splineWidths.push_back(*splineIt.width());
                        }
                        splineIt.next();
                    }
                    
                    UsdAttribute splinePointsAttr = splineCurves.CreatePointsAttr();
                    splinePointsAttr.Set<VtArray<GfVec3f>>(splineData.splinePoints, timeCode);

                    UsdAttribute splineVertexCountsAttr = splineCurves.CreateCurveVertexCountsAttr();
                    splineVertexCountsAttr.Set<VtArray<int>>(splineData.splineVertexCounts, timeCode);

                    UsdAttribute splineWidthsAttr = splineCurves.CreateWidthsAttr();
                    splineWidthsAttr.Set<VtArray<float>>(splineData.splineWidths, timeCode);

                    SdfValueTypeName uvValueType = SdfValueTypeNames->TexCoord2fArray;
                    UsdGeomPrimvar primvar = UsdGeomPrimvarsAPI(splineCurves).CreatePrimvar(TfToken("st1"),uvValueType);

                    UsdAttribute basisAttr = splineCurves.CreateBasisAttr();
                    basisAttr.Set(TfToken("bspline"));
                    primvar.Set(splineData.splineTexCoords, timeCode);
                    
                }
                
            }
        }
        splineDag.next();
    }
    
    */

    while(!itDag.isDone())
    {
        MFnDagNode nodeFn(itDag.item());
        MObject nodeObj = itDag.item();
        //MGlobal::displayInfo(nodeFn.name());
        if(nodeObj.hasFn(MFn::kMesh)){
            MGlobal::displayInfo(MString("Inside dag iterator!"));
            MString nodeType = nodeFn.typeName();
            MGlobal::displayInfo(nodeType);
            MDagPath path = nodeFn.dagPath();
            MFnMesh mesh(nodeObj);
            //MGlobal::displayInfo(nodeFn.name());
            nodeName = nodeFn.name().asChar();
            std::string nodePath = "/World/" + nodeName ;
            UsdGeomMesh usd_mesh = UsdGeomMesh::Define(stage, SdfPath(nodePath));
            
            MGlobal::displayInfo(MString("Created mesh prim!"));

            UsdUtilsSparseValueWriter* sparseWriter;
                   
            MPointArray pointArray;
            MFloatVectorArray pointNormalArray;
            // Points length = 0, have to check why
            mesh.getPoints(pointArray, MSpace::kObject);

            MBoundingBox boundingBox = nodeFn.boundingBox();
            MMatrix meshIncMatrix = path.inclusiveMatrix();
            boundingBox.transformUsing(meshIncMatrix);
            
            MVector bbVecMax = boundingBox.max();
            MVector bbVecMin = boundingBox.min();
            MVector bbVecCenter = boundingBox.center();

            GfVec3f extentMin(bbVecMin.x ,bbVecMin.y, bbVecMin.z);
            GfVec3f extentMax(bbVecMax.x ,bbVecMax.y, bbVecMax.z);
            GfVec3f extentCenter(bbVecCenter.x ,bbVecCenter.y, bbVecCenter.z);

            meshIndexData.extent.push_back(extentMin);
            meshIndexData.extent.push_back(extentCenter);
            meshIndexData.extent.push_back(extentMax);


            cout << pointArray.length() << endl;
            MGlobal::displayInfo(MString("Writing points !"));

            for(int i=0; i<pointArray.length();i++)
            {
                //pointsArr.push_back(GfVec3f(pointArray[i][0], pointArray[i][1], pointArray[i][2]));
                pointsArr.push_back({static_cast<float>(pointArray[i][0]), static_cast<float>(pointArray[i][1]), static_cast<float>(pointArray[i][2])});
            }
            mesh.getNormals(pointNormalArray, MSpace::kWorld);
            cout << pointNormalArray.length() << endl;
            for(int i=0; i<pointNormalArray.length();i++)
            {
                NormalsArr.push_back({static_cast<float>(pointNormalArray[i][0]), static_cast<float>(pointNormalArray[i][1]), static_cast<float>(pointNormalArray[i][2])});
            }

            getFaceVertexIndicesUVData(nodeObj, mesh);
            getTexCoords(mesh);

            MFnTransform transFn(nodeObj);
            MVector objTranslate = transFn.translation(MSpace::kWorld);
            MEulerRotation objRotate;
            transFn.getRotation(objRotate);



            MGlobal::displayInfo(MString("Mesh data written!"));

            //cout << meshIndexData.faceVertexCounts << endl;
           // cout << meshIndexData.faceVertexIndices << endl;
            //cout << meshIndexData.stIndices << endl;
            //cout << meshIndexData.texCoords << endl;

            //mesh.getVertices()
            // TODO::to gather face vertex indices and counts - https://forums.autodesk.com/t5/maya-programming/mfnmesh-getfacevertexindex-and-getfaceandvertexindices/m-p/8463920#M9172,
            // https://download.autodesk.com/us/maya/2010help/api/class_m_fn_mesh.html#38bf790e2ee0a16cda1b2199410cc0fd
        // mesh.getPolygonVertices()
            /*
            for(int i=0; i<pointArray.length();i++)
            {
                pointsVector.push_back(GfVec3f(pointArray[i][0], pointArray[i][1], pointArray[i][2]));
            }
            for(int i=0; i<pointNormalArray.length();i++)
            {
                NormalsArr.push_back(GfVec3f(pointArray[i][0], pointArray[i][1], pointArray[i][2]));
            }
            //sparseWriter->SetAttribute(pointsAttr, VtValue(pointsVector), timeCode);
            */
            UsdAttribute pointsAttr = usd_mesh.CreatePointsAttr();
            pointsAttr.Set<VtArray<GfVec3f>>(pointsArr, timeCode);

            UsdAttribute fvcAttr = usd_mesh.CreateFaceVertexCountsAttr();
            fvcAttr.Set<VtArray<int>>(meshIndexData.faceVertexCounts, timeCode);

            UsdAttribute fviAttr = usd_mesh.CreateFaceVertexIndicesAttr();
            fviAttr.Set<VtArray<int>>(meshIndexData.faceVertexIndices, timeCode);

            UsdAttribute normalsAttr = usd_mesh.CreateNormalsAttr();
            normalsAttr.Set<VtArray<GfVec3f>>(NormalsArr, timeCode);
            SdfValueTypeName uvValueType = SdfValueTypeNames->TexCoord2fArray;

            UsdAttribute extentAttr = usd_mesh.CreateExtentAttr();
            extentAttr.Set<VtArray<GfVec3f>>(meshIndexData.extent, timeCode);

            UsdGeomPrimvar primvar = UsdGeomPrimvarsAPI(usd_mesh).CreatePrimvar(TfToken("st"),uvValueType);

            primvar.SetIndices(meshIndexData.stIndices, timeCode);
            primvar.Set(meshIndexData.texCoords, timeCode);

            //usd_mesh.CreatePointsAttr(pointsVector);
            //usd_mesh.CreateNormalsAttr(VtValue(NormalsVector));
            break;
        }
        itDag.next();
    }
    stage->Save();
    return MStatus::kSuccess;

}
