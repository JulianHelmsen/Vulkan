#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H


#include "core/application.h"
#include "core/window.h"


#include "renderer/render_api.h"
#include "renderer/context.h"
#include "renderer/renderpass.h"
#include "renderer/pipeline.h"
#include "renderer/shader.h"
#include "renderer/framebuffer.h"
#include "renderer/command_buffer.h"
#include "renderer/synchronization.h"
#include "renderer/buffer.h"
#include "renderer/memory.h"


#define stack_array_len(arr) (sizeof(arr) / sizeof(arr[0]))

#endif //ENGINE_ENGINE_H