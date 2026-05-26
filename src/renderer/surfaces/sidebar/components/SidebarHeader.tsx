import { FiPlus } from "react-icons/fi";

export function SidebarHeader({
  onNewTab,
  workspaceName
}: {
  onNewTab: () => void;
  workspaceName: string;
}) {
  return (
    <header className="sidebar-header">
      <div>
        <p className="eyebrow">Workspace</p>
        <h1>{workspaceName}</h1>
      </div>
      <button className="icon-button" title="New tab" type="button" onClick={onNewTab}><FiPlus /></button>
    </header>
  );
}
