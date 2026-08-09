# RT-Thread to Odoo connector
this project connect RT-Thread board to Odoo, an enterprise resource planning (ERP) software
this project is adapted from built in example with its original **Documentation**|[**English**](README2.md)

# System Architecture
```mermaid
graph TD
    %% Define Styles
    classDef hardware fill:#006400,stroke:#333,stroke-width:2px;
    classDef software fill:#b7f,stroke:#333,stroke-width:2px;
    classDef cloud fill:#f96,stroke:#333,stroke-width:2px;

    %% Nodes Declaration
    Camera[Camera Module]:::hardware
    Titan[RT-Thread Titan Board]:::hardware
    Odoo[Custom Built Odoo App]:::software
    Qwen[Qwen LLM]:::cloud
    Alibaba[Alibaba Cloud Platform]:::cloud

    %% Connections
    Camera -->|Physical Connection| Titan
    Titan -->|Internet / HTTP| Odoo
    Odoo -->|API Call| Qwen
    Qwen --> Alibaba

    %% Layout grouping
    subgraph EdgeLayer["Edge Layer"]
        Camera
        Titan
    end

    subgraph ServerLayer["Server/ERP Layer"]
        Odoo
    end

    subgraph AILayer["AI Cloud Layer"]
        Alibaba
        Qwen
    end
```

