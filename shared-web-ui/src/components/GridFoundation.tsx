/**
 * GridFoundation - Core 4-column grid layout container.
 * All Section components must be placed inside a GridFoundation.
 * Sections flow automatically into rows based on their size (col-span).
 */

export type GridFoundationProps = {
  children: React.ReactNode;
};

export function GridFoundation({ children }: GridFoundationProps) {
  return <div className="grid grid-cols-4 gap-4">{children}</div>;
}
