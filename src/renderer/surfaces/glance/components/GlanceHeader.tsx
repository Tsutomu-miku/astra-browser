import {
  FiArrowLeft,
  FiArrowRight,
  FiColumns,
  FiExternalLink,
  FiRefreshCw,
  FiX
} from "react-icons/fi";

export interface GlanceNavigationState {
  canGoBack: boolean;
  canGoForward: boolean;
  isLoading: boolean;
}

export function GlanceHeader({
  navigation,
  onClose,
  onGoBack,
  onGoForward,
  onOpen,
  onRefresh,
  onSplit,
  title,
  url
}: {
  navigation: GlanceNavigationState;
  onClose: () => void;
  onGoBack: () => void;
  onGoForward: () => void;
  onOpen: () => void;
  onRefresh: () => void;
  onSplit: () => void;
  title: string;
  url: string;
}) {
  return (
    <header className="glance-header">
      <div className="glance-nav" aria-label="Glance navigation">
        <button
          className="icon-button"
          title="Back"
          type="button"
          disabled={!navigation.canGoBack}
          onClick={onGoBack}
        >
          <FiArrowLeft />
        </button>
        <button
          className="icon-button"
          title="Forward"
          type="button"
          disabled={!navigation.canGoForward}
          onClick={onGoForward}
        >
          <FiArrowRight />
        </button>
        <button className="icon-button" title="Reload" type="button" onClick={onRefresh}>
          <FiRefreshCw className={navigation.isLoading ? "is-spinning" : undefined} />
        </button>
      </div>
      <div className="glance-title-block">
        <p className="glance-kicker">Glance</p>
        <h2>{title}</h2>
        <span>{url}</span>
      </div>
      <div className="glance-actions">
        <button className="icon-button" title="Open in tab" type="button" onClick={onOpen}>
          <FiExternalLink />
        </button>
        <button className="icon-button" title="Open in split view" type="button" onClick={onSplit}>
          <FiColumns />
        </button>
        <button className="icon-button" title="Close Glance" type="button" onClick={onClose}>
          <FiX />
        </button>
      </div>
    </header>
  );
}
