import { initializeWAM } from "@/audio/wam-controller";

type StartWebAudioResult =
  | { success: true }
  | { success: false; error: string };

export async function startWebAudio(): Promise<StartWebAudioResult> {
  try {
    const wamController = await initializeWAM();

    if (!wamController) {
      return { success: false, error: "PluginController not found" };
    }

    window.Plugin_WAM = wamController;

    return { success: true };
  } catch (error) {
    console.error("Error starting web audio:", error);
    return {
      success: false,
      error: error instanceof Error ? error.message : String(error),
    };
  }
}




