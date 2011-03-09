
#ifndef __swfdec_marshal_MARSHAL_H__
#define __swfdec_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:BOOLEAN,POINTER,POINTER (./swfdec_marshal.list:1) */
extern void swfdec_marshal_BOOLEAN__BOOLEAN_POINTER_POINTER (GClosure     *closure,
                                                             GValue       *return_value,
                                                             guint         n_param_values,
                                                             const GValue *param_values,
                                                             gpointer      invocation_hint,
                                                             gpointer      marshal_data);

/* BOOLEAN:DOUBLE,DOUBLE,INT (./swfdec_marshal.list:2) */
extern void swfdec_marshal_BOOLEAN__DOUBLE_DOUBLE_INT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* BOOLEAN:UINT,UINT,BOOLEAN (./swfdec_marshal.list:3) */
extern void swfdec_marshal_BOOLEAN__UINT_UINT_BOOLEAN (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* VOID:POINTER,UINT (./swfdec_marshal.list:4) */
extern void swfdec_marshal_VOID__POINTER_UINT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:STRING,STRING (./swfdec_marshal.list:5) */
extern void swfdec_marshal_VOID__STRING_STRING (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:STRING,STRING,BOXED,UINT,BOXED,BOXED (./swfdec_marshal.list:6) */
extern void swfdec_marshal_VOID__STRING_STRING_BOXED_UINT_BOXED_BOXED (GClosure     *closure,
                                                                       GValue       *return_value,
                                                                       guint         n_param_values,
                                                                       const GValue *param_values,
                                                                       gpointer      invocation_hint,
                                                                       gpointer      marshal_data);

/* VOID:ULONG,UINT (./swfdec_marshal.list:7) */
extern void swfdec_marshal_VOID__ULONG_UINT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:ULONG,ULONG (./swfdec_marshal.list:8) */
extern void swfdec_marshal_VOID__ULONG_ULONG (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

G_END_DECLS

#endif /* __swfdec_marshal_MARSHAL_H__ */

