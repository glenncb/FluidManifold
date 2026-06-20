Import("env")
import shutil

print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", BUILD_TARGETS)

# invoke the pre action to create the modbusmap_gen files using Python
def pre_build_action(source, target, env):
    print("Calling pre_build_action to create modbusmap_gen files")
    program_path = target[0].get_abspath()
    print("Program path", program_path)
    print("Running genmodmap.py to generate modbusmap_gen files")
    env.Execute(f"python ./src/genmodmap.py")

env.AddPreAction("${BUILD_DIR}/src/modbusmap_gen.cpp.o", pre_build_action)
