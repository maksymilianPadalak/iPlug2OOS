/**
 * Contracts - Reference documentation for AI UI generation
 *
 * PHILOSOPHY: Full creative freedom.
 *
 * - AI can create ANY component
 * - AI can use ANY Tailwind classes
 * - AI can design ANY layout
 * - Users can request ANYTHING
 *
 * These contracts provide:
 * 1. Reference of what already exists (so AI doesn't recreate)
 * 2. Integration patterns (required to connect to audio engine)
 *
 * That's it. No constraints on creativity.
 */

// What components already exist (reference only)
export { existingComponents, componentImports } from '@/contracts/existingComponents';

// How to integrate with audio engine (required patterns)
export {
  parameterPatterns,
  midiPatterns,
  meterPatterns,
  visualizationPatterns,
  contextWrapper,
  availableHooks,
  parameterShapes,
} from '@/contracts/integrationPatterns';
