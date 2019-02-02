#include "SpatialTree.h"
#include "globals.h"
using namespace tinygltf;
using namespace std;
using namespace vcg;

SpatialTree::SpatialTree(Model* model, vector<MyMesh*> meshes)
{
    m_pModel = model;
    m_meshes = meshes;
    m_currentDepth = 0;
    m_pTileRoot = new TileInfo;
}


SpatialTree::~SpatialTree()
{
    deleteMyTreeNode(m_pRoot);
    deleteTileInfo(m_pTileRoot);
}

void SpatialTree::deleteTileInfo(TileInfo* tileInfo)
{
    if (tileInfo != NULL)
    {
        for (int i = 0; i < tileInfo->children.size(); ++i)
        {
            deleteTileInfo(tileInfo->children[i]);
        }
        delete tileInfo;
        tileInfo = NULL;
    }
}

TileInfo* SpatialTree::GetTilesetInfo()
{
    if (m_treeDepth < g_settings.tileLevel - 1)
    {
        for (int i = 0; i < g_settings.tileLevel - m_treeDepth - 1; ++i)
        {
            TileInfo* tileInfo = new TileInfo;
            tileInfo->boundingBox = m_pTileRoot->boundingBox;
            tileInfo->nodes = m_pTileRoot->nodes;
            tileInfo->children.push_back(m_pTileRoot);
            m_pTileRoot = tileInfo;
        }
        m_treeDepth = g_settings.tileLevel - 1;
    }
    recomputeTileBox(m_pTileRoot);
    return m_pTileRoot;
}

void SpatialTree::recomputeTileBox(TileInfo* parent)
{
    for (int i = 0; i < parent->children.size(); ++i)
    {
        recomputeTileBox(parent->children[i]);
    }
    parent->boundingBox->min.X() = parent->boundingBox->min.Y() = parent->boundingBox->min.Z() = INFINITY;
    parent->boundingBox->max.X() = parent->boundingBox->max.Y() = parent->boundingBox->max.Z() = -INFINITY;
    for (int i = 0; i < parent->myMeshInfos.size(); ++i)
    {
        parent->boundingBox->Add(parent->myMeshInfos[i].myMesh->bbox);
    }
}

void SpatialTree::deleteMyTreeNode(MyTreeNode* node)
{
    if (node != NULL)
    {
        if (node->left != NULL) 
        {
            deleteMyTreeNode(node->left);
        }
        if (node->right != NULL)
        {
            deleteMyTreeNode(node->right);
        }
        delete node;
        node = NULL;
    }
}

void SpatialTree::Initialize()
{
    Box3f* sceneBox = new Box3f();
    sceneBox->min.X() = sceneBox->min.Y() = sceneBox->min.Z() = INFINITY;
    sceneBox->max.X() = sceneBox->max.Y() = sceneBox->max.Z() = -INFINITY;
    
    m_pTileRoot = new TileInfo;
    
    for (int i = 0; i < m_meshes.size(); ++i)
    {
        sceneBox->Add(m_meshes[i]->bbox);
        MyMeshInfo meshInfo;
        meshInfo.myMesh = m_meshes[i];
        meshInfo.material = &(m_pModel->materials[m_pModel->meshes[i].primitives[0].material]);
        m_pTileRoot->myMeshInfos.push_back(meshInfo);
    }
    m_pTileRoot->boundingBox = sceneBox;

    splitTreeNode(m_pTileRoot);
}

void SpatialTree::splitTreeNode(TileInfo* parentTile)
{
    if (m_currentDepth > m_treeDepth) 
    {
        m_treeDepth = m_currentDepth;
    }

    m_currentDepth++;
    Point3f dim = parentTile->boundingBox->Dim();
    if (parentTile->myMeshInfos.size() < MIN_TREE_NODE || m_currentDepth > g_settings.maxTreeDepth)
    {
        m_currentDepth--;
        return;
    }

    TileInfo* pLeft = new TileInfo;
    TileInfo* pRight = new TileInfo;
    pLeft->boundingBox = new Box3f(*parentTile->boundingBox);
    pRight->boundingBox  = new Box3f(*parentTile->boundingBox);

    if (dim.X() > dim.Y() && dim.X() > dim.Z())
    {
        // Split X
        pLeft->boundingBox->max.X() = pRight->boundingBox->min.X() = parentTile->boundingBox->Center().X();
    }
    else if (dim.Y() > dim.X() && dim.Y() > dim.Z())
    {
        // Split Y
        pLeft->boundingBox->max.Y() = pRight->boundingBox->min.Y() = parentTile->boundingBox->Center().Y();
    }
    else
    {
        // Split Z
        pLeft->boundingBox->max.Z() = pRight->boundingBox->min.Z() = parentTile->boundingBox->Center().Z();
    }

    for (int i = 0; i < parentTile->myMeshInfos.size(); ++i)
    {
        MyMeshInfo meshInfo = parentTile->myMeshInfos[i];
        if (pLeft->boundingBox->IsInEx(meshInfo.myMesh->bbox.Center()))
        {
            pLeft->myMeshInfos.push_back(meshInfo);
        }
        else
        {
            pRight->myMeshInfos.push_back(meshInfo);
        }
    }

    // delete if children is empty
    if (pLeft->myMeshInfos.size() == 0)
    {
        delete pLeft;
        pLeft = NULL;
    }
    else
    {
        TileInfo* pLeftTile = new TileInfo;
        parentTile->children.push_back(pLeftTile);
        splitTreeNode(pLeftTile);
        parentTile->children.push_back(pLeft);
    }

    // delete if children is empty
    if (pRight->myMeshInfos.size() == 0)
    {
        delete pRight;
        pRight = NULL;
    }
    else
    {
        TileInfo* pRightTile = new TileInfo;
        parentTile->children.push_back(pRightTile);
        splitTreeNode(pRightTile);
        parentTile->children.push_back(pRight);
    }

    m_currentDepth--;
}
