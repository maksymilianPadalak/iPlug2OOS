/**
 * Runtime Parameters Provider
 *
 * Provides runtimeParameters to shared components (like Dropdown)
 * so they can look up enum options by paramId.
 */

import React, { createContext, useContext } from 'react';

/**
 * Minimal type for parameters that Dropdown needs.
 * Plugin-local RuntimeParameter types are supersets of this.
 */
export type RuntimeParameter = {
  id: number;
  name: string;
  enumValues?: string[] | null;
};

const RuntimeParametersContext = createContext<RuntimeParameter[]>([]);

export type RuntimeParametersProviderProps = {
  parameters: RuntimeParameter[];
  children: React.ReactNode;
};

export function RuntimeParametersProvider({ parameters, children }: RuntimeParametersProviderProps) {
  return (
    <RuntimeParametersContext.Provider value={parameters}>
      {children}
    </RuntimeParametersContext.Provider>
  );
}

export function useRuntimeParameters(): RuntimeParameter[] {
  return useContext(RuntimeParametersContext);
}
