#pragma once

//
// Spec-defined ACPI device IDs.
//
#define AML_ACPI_ID_EC_SMB_HC             "ACPI0001" /* EC SMBus Host Controller. */
#define AML_ACPI_ID_SMART_BATTERY         "ACPI0002" /* Smart Battery Object. */
#define AML_ACPI_ID_POWER_SOURCE          "ACPI0003" /* Power Source. */
#define AML_ACPI_ID_MODULE                "ACPI0004" /* Module. */
#define AML_ACPI_ID_SMB2_HC               "ACPI0005" /* SMBus 2.0 Host Controller. */
#define AML_ACPI_ID_GPE_BLOCK             "ACPI0006" /* GPE Block. */
#define AML_ACPI_ID_PROCESSOR             "ACPI0007" /* Processor. */
#define AML_ACPI_ID_AL_SENSOR             "ACPI0008" /* Ambient Light Sensor. */
#define AML_ACPI_ID_IO_XAPIC              "ACPI0009" /* Compatible with I/O APIC and I/O SAPIC. */
#define AML_ACPI_ID_IO_APIC               "ACPI000A" /* I/O APIC. */
#define AML_ACPI_ID_IO_SAPIC              "ACPI000B" /* I/O SAPIC. */			     
#define AML_ACPI_ID_PROCESSOR_AGGREGATOR  "ACPI000C" /* Processor Aggregator. */			     
#define AML_ACPI_ID_POWER_METER           "ACPI000D" /* Power Meter. */			     
#define AML_ACPI_ID_TW_ALARM              "ACPI000E" /* Time and Wake Alarm. */
#define AML_ACPI_ID_UP_DETECTION          "ACPI000F" /* User Presence Detection. */
#define AML_ACPI_ID_PROCESSOR_CONTAINER   "ACPI0010" /* Processor Container. */
#define AML_ACPI_ID_GENERIC_BUTTON        "ACPI0011" /* Generic Button. */
#define AML_ACPI_ID_NVDIMM                "ACPI0012" /* NVDIMM. */
#define AML_ACPI_ID_GENERIC_EVENT         "ACPI0013" /* Generic Event. */
#define AML_ACPI_ID_WIRELESS_POWER_CLB    "ACPI0014" /* Wireless Power Calibration. */
#define AML_ACPI_ID_USB4_HOST_INTERFACE   "ACPI0015" /* USB4 Host Interface. */
#define AML_ACPI_ID_CXL_HOST_BRIDGE       "ACPI0016" /* Compute Express Link Host Bridge. */
#define AML_ACPI_ID_CXL_ROOT              "ACPI0017" /* Compute Express Link Root Object. */
#define AML_ACPI_ID_AUDIO_COMPOSITION     "ACPI0018" /* Audio Composition. */

//
// PNP IDs/EISAID strings used internally.
//
#define AML_PNP_ID_PCI_ROOT_BUS  "PNP0A03" /* PCI Root Bus. */
#define AML_PNP_ID_PCIE_ROOT_BUS "PNP0A08" /* PCIe Root Bus. */

//
// Encoded EISAID values used internally.
//
#define AML_EISA_ID_PCI_ROOT_BUS  0x30AD041ul
#define AML_EISA_ID_PCIE_ROOT_BUS 0x80AD041ul