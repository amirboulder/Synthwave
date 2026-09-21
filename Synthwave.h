#pragma once

import Logger;

import TimeManager;

import GLM;
import MathComponents;

import Components;
import EventComponents;

import Phases;

import Util;

import AssetImporter;
import AssetManager;
import AssetLibrary;
import Manifest;

import EntityFactory;

import Jolt;
import PhysicsComponents;
import JoltAssetStream;
import PhysicsAnimation;
import PhysicsUtil;
import Ragdoll;

import PlayerComponents;
import Player;

import InputManager;
import InputComponents;

import Registry;

import Mesh;
import GeometryPool;
import RenderConfig;
import Camera;
import Pipeline;
import ShaderCompiler;
import ShaderReflection;
import GraphicsComponents;
import Texture;
import Material;
import RenderUtil;

#include "core/src/pch.h"

#include "core/src/renderer/renderer.hpp"

#include "core/src/TransformPropagation/TransformPropagation.hpp"

#include "game/src/gameObjects.hpp"

#include "core/src/physics/physics.hpp"

#include "core/src/AI/AI.hpp"

#include "core/src/MenuSystem/MenuSystem.hpp"

#include "Editor/src/editor.hpp"

#include "core/src/state/stateManager.hpp"

