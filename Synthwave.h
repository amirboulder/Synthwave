#pragma once

import Logger;
import ShaderCompiler;
import TimeManager;
import Jolt;
import GLM;
import PhysicsComponents;
import MathComponents;
import GraphicsComponents;
import Components;
import EventComponents;
import Util;
import Texture;
import Material;
import RenderUtil;
import AssetImporter;
import AssetManager;
import JoltAssetStream;
import PhysicsAnimation;


import Manifest;

//The following should only be needed in renderer module
import Mesh;
import GeometryPool;

#include "core/src/pch.h"

#include "core/src/common.hpp"

#include "core/src/renderer/renderer.hpp"

#include "core/src/TransformPropagation/TransformPropagation.hpp"

#include "game/src/gameObjects.hpp"

#include "core/src/physics/physics.hpp"

#include "core/src/AI/AI.hpp"

#include "core/src/EntityFactory.hpp"

#include "core/src/Registery/registry.hpp"

#include "core/src/InputSystem/InputManager.hpp"

#include "core/src/MenuSystem/MenuSystem.hpp"

#include "core/src/AssetSystems/AssetLibrary.hpp"

#include "Editor/src/editor.hpp"

#include "core/src/state/stateManager.hpp"

