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
import PhysicsUtil;
import AssetLibrary;
import EntityFactory;
import Ragdoll;
import Player;
import InputManager;
import InputComponents;

import Manifest;

//The following should only be needed in renderer module (once there is a render module)
import Mesh;
import GeometryPool;
import RenderConfig;
import Camera;
import Pipeline;
import ShaderReflection;

#include "core/src/pch.h"

#include "core/src/renderer/renderer.hpp"

#include "core/src/TransformPropagation/TransformPropagation.hpp"

#include "game/src/gameObjects.hpp"

#include "core/src/physics/physics.hpp"

#include "core/src/AI/AI.hpp"

#include "core/src/Registery/registry.hpp"

#include "core/src/MenuSystem/MenuSystem.hpp"

#include "Editor/src/editor.hpp"

#include "core/src/state/stateManager.hpp"

