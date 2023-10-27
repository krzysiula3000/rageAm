#pragma once

#ifdef AM_INTEGRATED

#include "lighteditor.h"
#include "mateditor.h"
#include "am/ui/app.h"
#include "am/integration/gameentity.h"
#include "am/asset/types/hotdrawable.h"

namespace rageam::integration
{
	// TODO: UI for archetype definition

	// Data shared between all components of the scene (scene graph, light editor, material editor etc)
	struct ModelSceneContext
	{
		bool					IsDrawableLoading;
		int						EntityHandle;
		Mat44V					EntityWorld;
		pVoid					EntityPtr;			// fwEntity
		amPtr<gtaDrawable>		Drawable;
		asset::DrawableAssetPtr DrawableAsset;
		AssetHotFlags			HotFlags;
		asset::HotTxdSet*		TXDs;				// All texture dictionaries in the workspace + embed

		auto GetDrawableMap() const { return DrawableAsset ? &DrawableAsset->CompiledDrawableMap : nullptr; }
		auto GetScene() const { return DrawableAsset ? DrawableAsset->GetScene().get() : nullptr; }
	};

	struct DrawableStats
	{
		Vec3S	Dimensions; // Size of drawable on each axis
		u32		Bones;
		u32		Lods;
		u32		Models;
		u32		Geometries;
		u32		Vertices;
		u32		Triangles;
		u32		Lights;

		static DrawableStats ComputeFrom(gtaDrawable* drawable);
	};

	static const Vec3V SCENE_ISOLATED_POS = { -1700, -6000, 310 };
	static const Vec3V SCENE_POS = { -676, 167, 73.55f };
	
	struct SceneGlue : IUpdateComponent
	{
		asset::HotDrawablePtr	HotDrawable;
		AssetHotFlags			HotFlags = AssetHotFlags_None;
		std::mutex				Mutex;

		void OnEarlyUpdate() override
		{
			std::unique_lock lock(Mutex);
			// Apply pending hot reload changes
			AssetHotFlags hotFlags = HotDrawable->ApplyChanges();
			HotFlags |= hotFlags;
		}
	};

	class ModelScene : public ui::App
	{
		enum eSceneNodeAttr
		{
			SceneNodeAttr_None,
			SceneNodeAttr_Mesh,			// rage::grmModel
			SceneNodeAttr_Bone,			// rage::crBoneData
			SceneNodeAttr_Collision,	// rage::phBound
			SceneNodeAttr_Light,		// CLightAttr
		};

		static constexpr ConstString SceneNodeAttrDisplay[] =
		{
			"None", "Mesh", "Bone", "Collision", "Light"
		};

		ComponentOwner<SceneGlue>	m_SceneGlue;
		ComponentOwner<GameEntity>	m_GameEntity;
		amPtr<CBaseArchetypeDef>	m_ArchetypeDef;
		ModelSceneContext			m_Context;
		LightEditor					m_LightEditor;
		MaterialEditor				m_MaterialEditor;
		bool						m_IsolatedSceneOn = false;
		Vec3V						m_ScenePosition = SCENE_POS; // NOTE: Can be overriden by user
		// Graph View
		s32							m_SelectedNodeIndex = -1;
		eSceneNodeAttr				m_SelectedNodeAttr = SceneNodeAttr_None;
		eSceneNodeAttr				m_JustSelectedNodeAttr = SceneNodeAttr_None; // In current frame
		DrawableStats				m_DrawableStats;
		SmallList<string>			m_CompilerMessages;
		double						m_CompilerProgress = 0.0;
		std::mutex					m_CompilerMutex;

		auto GetDrawableMap() const -> asset::DrawableAssetMap&;
		auto GetDrawable() const { return m_Context.Drawable.get(); }
		auto GetMeshAttr(u16 nodeIndex) const -> rage::grmModel*;
		auto GetBoneAttr(u16 nodeIndex) const -> rage::crBoneData*;
		auto GetBoundAttr(u16 nodeIndex) const -> rage::phBound*;
		auto GetLightAttr(u16 nodeIndex) const -> CLightAttr*;

		// Gets isolated/user scene position
		auto GetScenePosition() const -> Vec3V;
		void CreateArchetypeDefAndSpawnGameEntity();
		void WarpEntityToScenePosition();
		// Spawns entity and resets scene state
		void OnDrawableCompiled();
		void UpdateContextAndHotReloading();

		void DrawSceneGraphRecurse(const graphics::SceneNode* sceneNode);
		void DrawSceneGraph(const graphics::SceneNode* sceneNode);
		void DrawSkeletonGraph();

		void DrawNodePropertiesUI(u16 nodeIndex) const;
		void DrawDrawableUI();
		void OnRender() override;

	public:
		ModelScene();

		// Unloads currently loaded scene and destroys spawned drawable
		void Unload();
		void LoadFromPatch(ConstWString path);
		// Sets scene + camera position way outside of the map, isolating scene from game map
		// NOTE: This calls ResetCameraPosition after toggling!
		void SetIsolatedSceneOn(bool on);
		bool IsIsolatedSceneOn() const { return m_IsolatedSceneOn; }
		// Sets active debug camera (if there's any) position to target spawned prop
		void ResetCameraPosition() const;
	};
}

#endif
