import { initializeWAM, setupMIDIDevices } from "../../audio/wam-controller";

type StartWebAudioResult =
  | { success: true }
  | { success: false; error: string };

export async function startWebAudio(): Promise<StartWebAudioResult> {
  try {
    const wamController = await initializeWAM();

    if (!wamController) {
      return { success: false, error: "TemplateProjectController not found" };
    }

    window.TemplateProject_WAM = wamController;

    setTimeout(() => {
      setupMIDIDevices();
    }, 100);

    return { success: true };
  } catch (error) {
    console.error("Error starting web audio:", error);
    return {
      success: false,
      error: error instanceof Error ? error.message : String(error),
    };
  }
}


