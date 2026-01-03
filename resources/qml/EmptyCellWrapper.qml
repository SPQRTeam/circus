import QtQuick
import Circus 1.0

// Empty component for initializing all cell wrappers
// This ensures cell wrappers start with no assigned data source
ToolCellWrapper {
    id: emptyCellWrapper

    // Component starts with no stream assigned
    // Will be configured later via setStream() when user selects a data source
}
