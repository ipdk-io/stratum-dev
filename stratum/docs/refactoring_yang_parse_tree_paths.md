# Refactoring YangParseTreePaths

## 1. Background

The developers who implemented OpenConfig/gNMI support for DPDK in
P4-OVS made extensive changes to the implementation of the
*YangParseTreePaths* class (yang\_parse\_tree\_paths.cc).

  - Many of the paths were changed from */interfaces/interface* to
    */interfaces/virtual-interface*.
  - A number of *state* nodes were renamed to *config*.
  - A couple of dozen new port attributes (leaves) were added.

In the process, roughly 1500 lines of code were added to the file.
Further changes are planned.

## 2. Problem Statement

1.  yang\_parse\_tree\_paths is a common file. We cannot make arbitrary
    changes to it without affecting other targets; particularly not changes
    that alter existing paths.
2.  The unmodified file has \~4100 lines of code, which is unwieldy.
    The modified file has \~5600 lines, which is unmanageable.
3.  Additional changes are planned.

## 3. Proposed Solution

Refactor *yang\_parse\_tree\_paths.cc* into a number of smaller files,
each preferably having no more than 1000 lines of source code.

Maintain the order of the functions in each file.

  - This will allow the original file to be diffed with each of the
    derivative files to verify the integrity of the refactoring.

Ensure that the *AddSubtreeInterface* method and its supporting
functions are separated from the rest of the code.

  - This will allow us to replace *AddSubtreeInterface* and add new
    supporting functions by choosing a different selection of files in
    the DPDK build.
  - The DPDK-specific files will be in *stratum/hal/lib/tdi/dpdk*.

Move the YANG files to a new *stratum/hal/lib/yang* directory (paralleling
*stratum/hal/lib/p4*).

  - This avoids cluttering the *common* directory, which already has \~50
    non-YANG files in it.

## 4. Outcome

The result of the refactoring is as follows:

<table>
<tbody>
<tr class="odd">
<td>File</td>
<td>Description</td>
<td>Lines</td>
</tr>
<tr class="even">
<td>yang_add_subtree_interface.cc</td>
<td><em>AddSubtreeInterface method</em></td>
<td>231</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_chassis.cc</td>
<td><em>AddSubtreeChassis</em> and support functions</td>
<td>281</td>
</tr>
<tr class="even">
<td>yang_parse_tree_component.cc</td>
<td><em>SetUpComponentsComponent</em> support functions</td>
<td>152</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_component.h</td>
<td>Header file for yang_parse_tree_component.cc</td>
<td>44</td>
</tr>
<tr class="even">
<td>yang_parse_tree_helpers.cc</td>
<td>Helper functions. Formerly in an anonymous namespace;<br />
now in <em>::stratum::hal::yang::helpers</em>.</td>
<td>113</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_helpers.h</td>
<td>Header file for yang_parse_tree_helpers.cc</td>
<td>819</td>
</tr>
<tr class="even">
<td>yang_parse_tree_interface.cc</td>
<td>Support functions for <em>AddSubtreeInterface</em> method.<br />
Namespaced as <em>::stratum::hal::yang::interface</em></td>
<td>861</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_interface.h</td>
<td>Header file for yang_parse_tree_interface.cc</td>
<td>140</td>
</tr>
<tr class="even">
<td>yang_parse_tree_node.cc</td>
<td><em>AddSubtreeNode</em> and support functions</td>
<td>130</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_optical.cc</td>
<td><em>AddSubtreeInterfaceFromOptical</em> and support functions</td>
<td>704</td>
</tr>
<tr class="even">
<td>yang_parse_tree_paths.cc</td>
<td>Everything not refactored</td>
<td>335</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_paths.h</td>
<td>Header file for yang_parse_tree_paths.cc</td>
<td>79</td>
</tr>
<tr class="even">
<td>yang_parse_tree_singleton.cc</td>
<td><em>AddSubtreeInterfaceFromSingleton</em> and support functions</td>
<td>638</td>
</tr>
<tr class="odd">
<td>yang_parse_tree_system.cc</td>
<td><em>AddSubtreeSystem</em> and support functions</td>
<td>124</td>
</tr>
</tbody>
</table>

## 5. Further Changes

### 5.1 DPDK support

For DPDK, we expect to add the following to *stratum/hal/lib/tdi/dpdk*:

- dpdk_add_subtree_interface.cc
- dpdk_parse_tree_interface.cc
- dpdk_parse_tree_interface.h

### 5.2 Unit tests

It might be worth looking into refactoring *yang\_parse\_tree\_test.cc*.

  - At \~5600 lines, its size is excessive for a manually maintained
    file.
  - We might also (cough) want to add unit tests for the new
    functionality.

## 6. Alternatives

We could limit the refactoring to *AddSubtreeInterface* and its
support functions.

  - This would reduce the impact on the existing code, but it would do
    nothing for the maintainability of yang\_parse\_tree\_paths.cc.

We would still need to create the following files in order to support DPDK:

- yang_add_subtree_interface.cc
- yang_parse_tree_helpers.cc
- yang_parse_tree_helpers.h
- yang_parse_tree_interface.cc
- yang_parse_tree_interface.h
