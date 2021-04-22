/*
 * Copyright © 2010-2021 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Object distances.
 */

#ifndef HWLOC_DISTANCES_H
#define HWLOC_DISTANCES_H

#ifndef HWLOC_H
#error Please include the main hwloc.h instead
#endif


#ifdef __cplusplus
extern "C" {
#elif 0
}
#endif


/** \defgroup hwlocality_distances_get Retrieve distances between objects
 * @{
 */

/** \brief Matrix of distances between a set of objects.
 *
 * This matrix often contains latencies between NUMA nodes
 * (as reported in the System Locality Distance Information Table (SLIT)
 * in the ACPI specification), which may or may not be physically accurate.
 * It corresponds to the latency for accessing the memory of one node
 * from a core in another node.
 * The corresponding kind is ::HWLOC_DISTANCES_KIND_FROM_OS | ::HWLOC_DISTANCES_KIND_FROM_USER.
 * The name of this distances structure is "NUMALatency".
 * Others distance structures include and "XGMIBandwidth" and "NVLinkBandwidth".
 *
 * The matrix may also contain bandwidths between random sets of objects,
 * possibly provided by the user, as specified in the \p kind attribute.
 *
 * Pointers \p objs and \p values should not be replaced, reallocated, freed, etc.
 * However callers are allowed to modify \p kind as well as the contents
 * of \p objs and \p values arrays.
 * For instance, if there is a single NUMA node per Package,
 * hwloc_get_obj_with_same_locality() may be used to convert between them
 * and replace NUMA nodes in the \p objs array with the corresponding Packages.
 */
struct hwloc_distances_s {
  unsigned nbobjs;		/**< \brief Number of objects described by the distance matrix. */
  hwloc_obj_t *objs;		/**< \brief Array of objects described by the distance matrix.
				 * These objects are not in any particular order,
				 * see hwloc_distances_obj_index() and hwloc_distances_obj_pair_values()
				 * for easy ways to find objects in this array and their corresponding values.
				 */
  unsigned long kind;		/**< \brief OR'ed set of ::hwloc_distances_kind_e. */
  hwloc_uint64_t *values;	/**< \brief Matrix of distances between objects, stored as a one-dimension array.
				 *
				 * Distance from i-th to j-th object is stored in slot i*nbobjs+j.
				 * The meaning of the value depends on the \p kind attribute.
				 */
};

/** \brief Kinds of distance matrices.
 *
 * The \p kind attribute of struct hwloc_distances_s is a OR'ed set
 * of kinds.
 *
 * A kind of format HWLOC_DISTANCES_KIND_FROM_* specifies where the
 * distance information comes from, if known.
 *
 * A kind of format HWLOC_DISTANCES_KIND_MEANS_* specifies whether
 * values are latencies or bandwidths, if applicable.
 */
enum hwloc_distances_kind_e {
  /** \brief These distances were obtained from the operating system or hardware.
   * \hideinitializer
   */
  HWLOC_DISTANCES_KIND_FROM_OS = (1UL<<0),
  /** \brief These distances were provided by the user.
   * \hideinitializer
   */
  HWLOC_DISTANCES_KIND_FROM_USER = (1UL<<1),

  /** \brief Distance values are similar to latencies between objects.
   * Values are smaller for closer objects, hence minimal on the diagonal
   * of the matrix (distance between an object and itself).
   * It could also be the number of network hops between objects, etc.
   * \hideinitializer
   */
  HWLOC_DISTANCES_KIND_MEANS_LATENCY = (1UL<<2),
  /** \brief Distance values are similar to bandwidths between objects.
   * Values are higher for closer objects, hence maximal on the diagonal
   * of the matrix (distance between an object and itself).
   * Such values are currently ignored for distance-based grouping.
   * \hideinitializer
   */
  HWLOC_DISTANCES_KIND_MEANS_BANDWIDTH = (1UL<<3),

  /** \brief This distances structure covers objects of different types.
   * This may apply to the "NVLinkBandwidth" structure in presence
   * of a NVSwitch or POWER processor NVLink port.
   * \hideinitializer
   */
  HWLOC_DISTANCES_KIND_HETEROGENEOUS_TYPES = (1UL<<4)
};

/** \brief Retrieve distance matrices.
 *
 * Retrieve distance matrices from the topology into the \p distances array.
 *
 * \p flags is currently unused, should be \c 0.
 *
 * \p kind serves as a filter. If \c 0, all distance matrices are returned.
 * If it contains some HWLOC_DISTANCES_KIND_FROM_*, only distance matrices
 * whose kind matches one of these are returned.
 * If it contains some HWLOC_DISTANCES_KIND_MEANS_*, only distance matrices
 * whose kind matches one of these are returned.
 *
 * On input, \p nr points to the number of distance matrices that may be stored
 * in \p distances.
 * On output, \p nr points to the number of distance matrices that were actually
 * found, even if some of them couldn't be stored in \p distances.
 * Distance matrices that couldn't be stored are ignored, but the function still
 * returns success (\c 0). The caller may find out by comparing the value pointed
 * by \p nr before and after the function call.
 *
 * Each distance matrix returned in the \p distances array should be released
 * by the caller using hwloc_distances_release().
 */
HWLOC_DECLSPEC int
hwloc_distances_get(hwloc_topology_t topology,
		    unsigned *nr, struct hwloc_distances_s **distances,
		    unsigned long kind, unsigned long flags);

/** \brief Retrieve distance matrices for object at a specific depth in the topology.
 *
 * Identical to hwloc_distances_get() with the additional \p depth filter.
 */
HWLOC_DECLSPEC int
hwloc_distances_get_by_depth(hwloc_topology_t topology, int depth,
			     unsigned *nr, struct hwloc_distances_s **distances,
			     unsigned long kind, unsigned long flags);

/** \brief Retrieve distance matrices for object of a specific type.
 *
 * Identical to hwloc_distances_get() with the additional \p type filter.
 */
HWLOC_DECLSPEC int
hwloc_distances_get_by_type(hwloc_topology_t topology, hwloc_obj_type_t type,
			    unsigned *nr, struct hwloc_distances_s **distances,
			    unsigned long kind, unsigned long flags);

/** \brief Retrieve a distance matrix with the given name.
 *
 * Usually only one distances structure may match a given name.
 *
 * The name of the most common structure is "NUMALatency".
 * Others include "XGMIBandwidth" and "NVLinkBandwidth".
 */
HWLOC_DECLSPEC int
hwloc_distances_get_by_name(hwloc_topology_t topology, const char *name,
			    unsigned *nr, struct hwloc_distances_s **distances,
			    unsigned long flags);

/** \brief Get a description of what a distances structure contains.
 *
 * For instance "NUMALatency" for hardware-provided NUMA distances (ACPI SLIT),
 * or NULL if unknown.
 */
HWLOC_DECLSPEC const char *
hwloc_distances_get_name(hwloc_topology_t topology, struct hwloc_distances_s *distances);

/** \brief Release a distance matrix structure previously returned by hwloc_distances_get().
 *
 * \note This function is not required if the structure is removed with hwloc_distances_release_remove().
 */
HWLOC_DECLSPEC void
hwloc_distances_release(hwloc_topology_t topology, struct hwloc_distances_s *distances);

/** \brief Transformations of distances structures. */
enum hwloc_distances_transform_e {
  /** \brief Remove \c NULL objects from the distances structure.
   *
   * Every object that was replaced with \c NULL in the \p objs array
   * is removed and the \p values array is updated accordingly.
   *
   * At least \c 2 objects must remain, otherwise hwloc_distances_transform()
   * will return \c -1 with \p errno set to \c EINVAL.
   *
   * \p kind will be updated with or without ::HWLOC_DISTANCES_KIND_HETEROGENEOUS_TYPES
   * according to the remaining objects.
   *
   * \hideinitializer
   */
  HWLOC_DISTANCES_TRANSFORM_REMOVE_NULL = 0,

  /** \brief Replace bandwidth values with a number of links.
   *
   * Usually all values will be either \c 0 (no link) or \c 1 (one link).
   * However some matrices could get larger values if some pairs of
   * peers are connected by different numbers of links.
   *
   * Values on the diagonal are set to \c 0.
   *
   * This transformation only applies to bandwidth matrices.
   *
   * \hideinitializer
   */
  HWLOC_DISTANCES_TRANSFORM_LINKS = 1
};

/** \brief Apply a transformation to a distances structure.
 *
 * Modify a distances structure that was previously obtained with
 * hwloc_distances_get() or one of its variants.
 *
 * This modifies the local copy of the distances structures but does
 * not modify the distances information stored inside the topology
 * (retrieved by another call to hwloc_distances_get() or exported to XML).
 * To do so, one should add a new distances structure with same
 * name, kind, objects and values (see \ref hwlocality_distances_add)
 * and then remove this old one with hwloc_distances_release_remove().
 *
 * \p transform must be one of the transformations listed
 * in ::hwloc_distances_transform_e.
 *
 * These transformations may modify the contents of the \p objs or \p values arrays.
 *
 * \p transform_attr must be \c NULL for now.
 *
 * \p flags must be \c 0 for now.
 */
HWLOC_DECLSPEC int hwloc_distances_transform(hwloc_topology_t topology, struct hwloc_distances_s *distances,
                                             enum hwloc_distances_transform_e transform,
                                             void *transform_attr,
                                             unsigned long flags);

/** @} */



/** \defgroup hwlocality_distances_consult Helpers for consulting distance matrices
 * @{
 */

/** \brief Find the index of an object in a distances structure.
 *
 * \return -1 if object \p obj is not involved in structure \p distances.
 */
static __hwloc_inline int
hwloc_distances_obj_index(struct hwloc_distances_s *distances, hwloc_obj_t obj)
{
  unsigned i;
  for(i=0; i<distances->nbobjs; i++)
    if (distances->objs[i] == obj)
      return (int)i;
  return -1;
}

/** \brief Find the values between two objects in a distance matrices.
 *
 * The distance from \p obj1 to \p obj2 is stored in the value pointed by
 * \p value1to2 and reciprocally.
 *
 * \return -1 if object \p obj1 or \p obj2 is not involved in structure \p distances.
 */
static __hwloc_inline int
hwloc_distances_obj_pair_values(struct hwloc_distances_s *distances,
				hwloc_obj_t obj1, hwloc_obj_t obj2,
				hwloc_uint64_t *value1to2, hwloc_uint64_t *value2to1)
{
  int i1 = hwloc_distances_obj_index(distances, obj1);
  int i2 = hwloc_distances_obj_index(distances, obj2);
  if (i1 < 0 || i2 < 0)
    return -1;
  *value1to2 = distances->values[i1 * distances->nbobjs + i2];
  *value2to1 = distances->values[i2 * distances->nbobjs + i1];
  return 0;
}

/** @} */



/** \defgroup hwlocality_distances_add Add distances between objects
 *
 * The usual way to add distances is:
 * \code
 * hwloc_distances_add_handle_t handle;
 * int err = -1;
 * handle = hwloc_distances_add_create(topology, "name", kind, 0);
 * if (handle) {
 *   err = hwloc_distances_add_values(topology, handle, nbobjs, objs, values, 0);
 *   if (!err)
 *     err = hwloc_distances_add_commit(topology, handle, flags);
 * }
 * \endcode
 * If \p err is \c 0 at the end, then addition was successful.
 *
 * @{
 */

/** \brief Handle to a new distances structure during its addition to the topology. */
typedef void * hwloc_distances_add_handle_t;

/** \brief Create a new empty distances structure.
 *
 * Create an empty distances structure
 * to be filled with hwloc_distances_add_values()
 * and then committed with hwloc_distances_add_commit().
 *
 * Parameter \p name is optional, it may be \c NULL.
 * Otherwise, it will be copied internally and may later be freed by the caller.
 *
 * \p kind specifies the kind of distance as a OR'ed set of ::hwloc_distances_kind_e.
 * Kind ::HWLOC_DISTANCES_KIND_HETEROGENEOUS_TYPES will be automatically set
 * according to objects having different types in hwloc_distances_add_values().
 *
 * \p flags must be \c 0 for now.
 *
 * \return A hwloc_distances_add_handle_t that should then be passed
 * to hwloc_distances_add_values() and hwloc_distances_add_commit().
 *
 * \return \c NULL on error.
 */
HWLOC_DECLSPEC hwloc_distances_add_handle_t
hwloc_distances_add_create(hwloc_topology_t topology,
                           const char *name, unsigned long kind,
                           unsigned long flags);

/** \brief Specify the objects and values in a new empty distances structure.
 *
 * Specify the objects and values for a new distances structure
 * that was returned as a handle by hwloc_distances_add_create().
 * The structure must then be committed with hwloc_distances_add_commit().
 *
 * The number of objects is \p nbobjs and the array of objects is \p objs.
 * Distance values are stored as a one-dimension array in \p values.
 * The distance from object i to object j is in slot i*nbobjs+j.
 *
 * \p nbobjs must be at least 2.
 *
 * Arrays \p objs and \p values will be copied internally,
 * they may later be freed by the caller.
 *
 * On error, the temporary distances structure and its content are destroyed.
 *
 * \p flags must be \c 0 for now.
 *
 * \return \c 0 on success.
 * \return \c -1 on error.
 */
HWLOC_DECLSPEC int hwloc_distances_add_values(hwloc_topology_t topology,
                                              hwloc_distances_add_handle_t handle,
                                              unsigned nbobjs, hwloc_obj_t *objs,
                                              hwloc_uint64_t *values,
                                              unsigned long flags);

/** \brief Flags for adding a new distances to a topology. */
enum hwloc_distances_add_flag_e {
  /** \brief Try to group objects based on the newly provided distance information.
   * This is ignored for distances between objects of different types.
   * \hideinitializer
   */
  HWLOC_DISTANCES_ADD_FLAG_GROUP = (1UL<<0),
  /** \brief If grouping, consider the distance values as inaccurate and relax the
   * comparisons during the grouping algorithms. The actual accuracy may be modified
   * through the HWLOC_GROUPING_ACCURACY environment variable (see \ref envvar).
   * \hideinitializer
   */
  HWLOC_DISTANCES_ADD_FLAG_GROUP_INACCURATE = (1UL<<1)
};

/** \brief Commit a new distances structure.
 *
 * This function finalizes the distances structure and inserts in it the topology.
 *
 * Parameter \p handle was previously returned by hwloc_distances_add_create().
 * Then objects and values were specified with hwloc_distances_add_values().
 *
 * \p flags configures the behavior of the function using an optional OR'ed set of
 * ::hwloc_distances_add_flag_e.
 * It may be used to request the grouping of existing objects based on distances.
 *
 * On error, the temporary distances structure and its content are destroyed.
 *
 * \return \c 0 on success.
 * \return \c -1 on error.
 */
HWLOC_DECLSPEC int hwloc_distances_add_commit(hwloc_topology_t topology,
                                              hwloc_distances_add_handle_t handle,
                                              unsigned long flags);

/** @} */



/** \defgroup hwlocality_distances_remove Remove distances between objects
 * @{
 */

/** \brief Remove all distance matrices from a topology.
 *
 * Remove all distance matrices, either provided by the user or
 * gathered through the OS.
 *
 * If these distances were used to group objects, these additional
 * Group objects are not removed from the topology.
 */
HWLOC_DECLSPEC int hwloc_distances_remove(hwloc_topology_t topology);

/** \brief Remove distance matrices for objects at a specific depth in the topology.
 *
 * Identical to hwloc_distances_remove() but only applies to one level of the topology.
 */
HWLOC_DECLSPEC int hwloc_distances_remove_by_depth(hwloc_topology_t topology, int depth);

/** \brief Remove distance matrices for objects of a specific type in the topology.
 *
 * Identical to hwloc_distances_remove() but only applies to one level of the topology.
 */
static __hwloc_inline int
hwloc_distances_remove_by_type(hwloc_topology_t topology, hwloc_obj_type_t type)
{
  int depth = hwloc_get_type_depth(topology, type);
  if (depth == HWLOC_TYPE_DEPTH_UNKNOWN || depth == HWLOC_TYPE_DEPTH_MULTIPLE)
    return 0;
  return hwloc_distances_remove_by_depth(topology, depth);
}

/** \brief Release and remove the given distance matrice from the topology.
 *
 * This function includes a call to hwloc_distances_release().
 */
HWLOC_DECLSPEC int hwloc_distances_release_remove(hwloc_topology_t topology, struct hwloc_distances_s *distances);

/** @} */


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_DISTANCES_H */
