#include <vector>

#include <maya/MSimple.h>
#include <maya/MLibrary.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MFnDagNode.h>
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

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/mesh.h>
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

    UsdStageRefPtr stage = UsdStage::CreateNew("D:/Groom_usd_test/Files_usd/mesh.usda");
    MGlobal::displayInfo(MString("Created stage!"));
    UsdGeomXform root_prim = UsdGeomXform::Define(stage, SdfPath("/World"));
    MGlobal::displayInfo(MString("Created Xform!"));


    while(!itDag.isDone())
    {
        MFnDagNode nodeFn(itDag.item());
        MObject nodeObj = itDag.item();
        if(nodeObj.hasFn(MFn::kMesh)){
            MGlobal::displayInfo(MString("Inside dag iterator!"));
            MString nodeType = nodeFn.typeName();
            MGlobal::displayInfo(nodeType);
            MDagPath path = nodeFn.dagPath();
            MFnMesh mesh(nodeObj);
            MGlobal::displayInfo(nodeFn.name());
            nodeName = nodeFn.name().asChar();
            std::string nodePath = "/World/" + nodeName ;
            UsdGeomMesh usd_mesh = UsdGeomMesh::Define(stage, SdfPath(nodePath));
            UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
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

            stage->Save();
            break;
        }
        itDag.next();
        /*
        unsigned int grp_children_count = nodeFn.childCount();
        //Filtering camera nodes
        for (int i=0; i < grp_children_count; i++){
            MObject child = nodeFn.child(i);


            MFnDagNode childNodeFn(child);

            if(childNodeFn.type() == MFn::kMesh){
                MDagPath path = nodeFn.dagPath();
                MFnMesh mesh(path);
                MGlobal::displayInfo(nodeFn.name());
                nodeName = nodeFn.name().asChar();

                std::string nodePath = "/World/" + nodeName ;
                UsdGeomMesh usd_mesh = UsdGeomMesh::Define(stage, SdfPath(nodePath));

                MPointArray pointArray;
                MFloatVectorArray pointNormalArray;
                mesh.getPoints(pointArray, MSpace::kWorld);
                mesh.getNormals(pointNormalArray, MSpace::kWorld);
                // TODO::to gather face vertex indices and counts - https://forums.autodesk.com/t5/maya-programming/mfnmesh-getfacevertexindex-and-getfaceandvertexindices/m-p/8463920#M9172,
                // https://download.autodesk.com/us/maya/2010help/api/class_m_fn_mesh.html#38bf790e2ee0a16cda1b2199410cc0fd
            // mesh.getPolygonVertices()
                for(int i=0; i<pointArray.length();i++)
                {
                    pointsVector.push_back(GfVec3f(pointArray[i][0], pointArray[i][1], pointArray[i][2]));
                }
                for(int i=0; i<pointNormalArray.length();i++)
                {
                    NormalsVector.push_back(GfVec3f(pointArray[i][0], pointArray[i][1], pointArray[i][2]));
                }
                MGlobal::displayInfo(nodeFn.name());
                nodeName = nodeFn.name().asChar();

                usd_mesh.CreatePointsAttr(VtValue(pointsVector));
                usd_mesh.CreateNormalsAttr(VtValue(NormalsVector));

                stage->Save();

                break;
                
                //groupDagNodes.push_back(nodeFn);
            }

        }
        //MObject curItem = itDag.currentItem();
        //MFnDependencyNode fnNode;

        //fnNode.setObject(curItem);

        }

    */
    }
    return MStatus::kSuccess;

}
