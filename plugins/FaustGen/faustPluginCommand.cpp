#include "faustPluginCommand.hpp"
#include "faustPluginParsing.hpp"
#include "faust/gui/SoundUI.h"

namespace FaustGen {
    
std::string defaultSoundfilesDirectory()
{
    char* soundfiles_dir = getenv("FAUST_SOUNDFILES");
    return std::string((soundfiles_dir) ? soundfiles_dir : "");
}

#ifdef __APPLE__
std::string defaultUserAppSupportDirectory()
{
    return std::string(getenv("HOME")) + "/Library/Application Support/SuperCollider/Extensions";
}
std::string defaultSoundfilesDirectory1()
{
    return std::string(getenv("HOME")) + "/Library/Application Support/SuperCollider/Extensions/FaustSounds";
}
#else
std::string defaultUserAppSupportDirectory()
{
    return getenv("HOME");
}
std::string defaultSoundfilesDirectory1()
{
    return std::string(getenv("HOME")) + "/FaustSounds";
}
#endif


extern const bool debug_messages;

FaustData faustData;
    
static SoundUI* gSoundInterface = nullptr;

/**********************************************
 *
 * PLUGIN COMMUNICATION
 *
 * *******************************************/

// PARSE CODE AND CREATE DSP
// stage2 is non real time
bool cmdStage2(World *world, void *inUserData) {
  FaustCommandData *faustCmdData = (FaustCommandData *)inUserData;

  faustCmdData->parsedOK = parse(faustCmdData);

  // creating the DSP instance for interfacing
  if (faustCmdData->parsedOK) {
    faustCmdData->commandDsp = faustCmdData->factory->createDSPInstance();
      
    if (!gSoundInterface) {
        std::vector<std::string> soundfile_dirs = { defaultUserAppSupportDirectory(), defaultSoundfilesDirectory1(), SoundUI::getBinaryPath() };
        gSoundInterface = new SoundUI(soundfile_dirs);
    }
      
    faustCmdData->commandDsp->buildUserInterface(gSoundInterface);
      
    faustCmdData->commandDsp->init(static_cast<int>(faustCmdData->sampleRate));

    return true;
  } else {
    return false;
  }
}

// PASS DSP TO UNIT
// stage3 is real time - completion msg performed if stage3 returns true
bool cmdStage3(World *world, void *inUserData) {
  FaustCommandData *faustCmdData = (FaustCommandData *)inUserData;

  auto thisId = faustCmdData->id;
  auto instance = faustData.instances.at(thisId);
  if (instance) {
    instance->setNewDSP(faustCmdData->commandDsp);
    return true;
  } else {
    return false;
  }
}

// stage4 is non real time - sends done if stage4 returns true
bool cmdStage4(World *world, void *inUserData) { return true; }

void cmdCleanup(World *world, void *inUserData) {
  FaustCommandData *faustCmdData = (FaustCommandData *)inUserData;

  RTFree(world, faustCmdData->code); // free the string
  // @TODO will this delete factory and dsp as well, properly?
  RTFree(world, faustCmdData); // free command data
  // scsynth will delete the completion message for you.
}

void receiveNewFaustCode(World *inWorld, void *inUserData,
                         struct sc_msg_iter *args, void *replyAddr) {

  // allocate command data, free it in cmdCleanup.
  FaustCommandData *faustCmdData =
      (FaustCommandData *)RTAlloc(inWorld, sizeof(FaustCommandData));

  faustCmdData->sampleRate = inWorld->mSampleRate;

  // ID arguments
  faustCmdData->id = args->geti();
  faustCmdData->nodeID = args->geti();

  const char *newCode = args->gets(); // get the string argument
  if (newCode) {
    faustCmdData->code = (char *)RTAlloc(inWorld, strlen(newCode) + 1);
    // allocate space, free it in cmdCleanup. */
    strcpy(faustCmdData->code, newCode);

    std::cout << "Received new code: " << newCode << std::endl;
  }

  // how to pass a completion message
  int msgSize = args->getbsize();
  char *msgData = 0;
  if (msgSize) {
    // allocate space for completion message scsynth will delete the completion
    // message for you.
    msgData = (char *)RTAlloc(inWorld, msgSize);
    // copy completion message.
    args->getb(msgData, msgSize);
  }

  DoAsynchronousCommand(inWorld, replyAddr, "fausteval", (void *)faustCmdData,
                        (AsyncStageFn)cmdStage2, (AsyncStageFn)cmdStage3,
                        (AsyncStageFn)cmdStage4, cmdCleanup, msgSize, msgData);
}

} // namespace FaustGen
