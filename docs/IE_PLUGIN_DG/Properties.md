# Plugin Properties {#openvino_docs_ov_plugin_dg_properties}

@sphinxdirective

Plugin can provide own device-specific properties.

Property Class
##############

OpenVINO API provides the interface ov::Property which allows to define the property and access rights. Based on that, a declaration of plugin specific properties can look as follows: 

.. doxygensnippet:: src/plugins/template/include/template/properties.hpp
   :language: cpp
   :fragment: [properties:public_header]

@endsphinxdirective

