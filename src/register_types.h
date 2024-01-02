#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;

void initialize_whisper_module(ModuleInitializationLevel p_level);
void uninitialize_whisper_module(ModuleInitializationLevel p_level);

#endif // REGISTER_TYPES_H
